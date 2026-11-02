#include "app_runtime.h"
#include "app_config.h"
#include "app_board.h"
#include "bike_core.h"
#include "gnss_parser.h"
#include "network_service.h"
#include "legacy_reader.h"
#include "navigation_core.h"
#include "queue.h"
#include "task.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
static QueueHandle_t storage_queue,ack_queue,ui_queue,telemetry_queue,sensor_queue,nav_queue,control_queue,restore_queue;
static TaskHandle_t control_handle,service_handle,ui_handle;
typedef struct {BikeSnapshot bike;GpsFix fix;} Telemetry;
typedef struct {RideSummary summary;uint32_t elapsed_ms;} RestoreRequest;
typedef struct {NavStep step;uint32_t version,remaining_m;unsigned index,count;} NavView;
static BikeCore bike;
static GnssParser gps;
static uint32_t gps_utc_base,gps_utc_ms;
static StorageService storage;
static NetworkService network;
static uint32_t timestamp_from_fix(const GpsFix *f) {
    uint32_t y=2000+f->date_ddmmyy%100,m=f->date_ddmmyy/100%100,d=f->date_ddmmyy/10000,days=0,i;
    static const uint8_t months[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(!m||m>12||!d)return 0;
    for(i=1970;i<y;i++)days+=365+(i%4==0&&(i%100!=0||i%400==0));
    for(i=1;i<m;i++)days+=months[i-1]+(i==2&&y%4==0);
    return (days+d-1)*86400+f->utc_seconds;
}
static void sample(StorageOp op,const SensorSnapshot *sensor) {
    StorageRequest request;StorageAck fail;
    BikeCore_MakeRequest(&bike,&request,op,gps_utc_base?gps_utc_base+(uint32_t)(HAL_GetTick()-gps_utc_ms)/1000:0);
    (void)sensor;
    /* FIFO order, reserve one entry for STOP; no overwrite for persistent data. */
    if((op==STORE_SAMPLE&&uxQueueSpacesAvailable(storage_queue)<=1)||xQueueSend(storage_queue,&request,0)!=pdTRUE) {
        app_health.storage_queue_full++;bike.view.storage_overflows++;
        if(op==STORE_STOP) {fail.request_id=request.request_id;fail.result=APP_FULL;fail.address=0;BikeCore_Ack(&bike,&fail);}
    }
}
static void health(void) {
    app_health.heap_free=(uint32_t)xPortGetFreeHeapSize();app_health.heap_min=(uint32_t)xPortGetMinimumEverFreeHeapSize();
    if(control_handle)app_health.control_stack=(uint32_t)uxTaskGetStackHighWaterMark(control_handle);
    if(service_handle)app_health.service_stack=(uint32_t)uxTaskGetStackHighWaterMark(service_handle);
    if(ui_handle)app_health.ui_stack=(uint32_t)uxTaskGetStackHighWaterMark(ui_handle);
}
static void ControlTask(void *arg) {
    TickType_t wake=xTaskGetTickCount();uint32_t now,last=HAL_GetTick(),last_sample=last,last_health=last,last_sensor=0,last_overflow=0;
    uint8_t bytes[64],key_count[2]={0,0},key_latched[2]={0,0},cmd;size_t count,i;int event;
    StorageAck ack;SensorSnapshot sensor;Telemetry snapshot;RestoreRequest restore;
    (void)arg;memset(&sensor,0,sizeof(sensor));BikeCore_Init(&bike,last);GnssParser_Reset(&gps);
    App_UartStart(1);App_UartStart(3);
    for(;;) {
        now=HAL_GetTick();if(now-last>app_health.control_latency_max_ms)app_health.control_latency_max_ms=now-last;last=now;
        BikeCore_Tick(&bike,now);
        if(last_overflow!=app_health.gps_rx_overflow) {last_overflow=app_health.gps_rx_overflow;gps.collecting=0;gps.length=0;}
        count=xStreamBufferReceive(app_gps_stream,bytes,sizeof(bytes),0);
        for(i=0;i<count;i++) {GpsFix fix;if(GnssParser_Feed(&gps,bytes[i],app_gps_last_rx_ms,&fix)==1) {BikeCore_Gps(&bike,&fix);if(fix.valid) {gps_utc_base=timestamp_from_fix(&fix);gps_utc_ms=fix.received_ms;}}}
        bike.view.gps_valid=(uint8_t)GpsFix_IsFresh(&bike.fix,now);
        app_health.gps_good=gps.good;app_health.gps_bad=gps.bad;app_health.gps_oversized=gps.oversized;
        while(xQueueReceive(ack_queue,&ack,0)==pdTRUE) {if(!ack.request_id&&ack.result)bike.view.storage_errors++;else BikeCore_Ack(&bike,&ack);}
        (void)xQueueReceive(sensor_queue,&sensor,0);
        bike.view.heading=sensor.heading;bike.view.mag_valid=sensor.mag_valid;
        if(!sensor.aht_valid)BikeCore_Sensor(&bike,0,0,0);
        if(sensor.updated_ms!=last_sensor) {last_sensor=sensor.updated_ms;BikeCore_Sensor(&bike,sensor.temperature,sensor.humidity,sensor.aht_valid);}
        if(xQueueReceive(restore_queue,&restore,0)==pdTRUE)(void)BikeCore_Restore(&bike,&restore.summary,restore.elapsed_ms,now,IS_RESTORE);
        for(i=0;i<2;i++) {
            int pressed=HAL_GPIO_ReadPin(i?GPIOB:GPIOC,i?GPIO_PIN_0:GPIO_PIN_14)==GPIO_PIN_RESET;
            if(pressed) {if(key_count[i]<3)key_count[i]++;}
            else {if(key_count[i]>=2&&key_latched[i]) {cmd=(uint8_t)i;if(xQueueSend(control_queue,&cmd,0)!=pdTRUE)app_health.uart_errors++;}key_count[i]=0;key_latched[i]=0;}
            if(key_count[i]>=2)key_latched[i]=1;
        }
        while(xQueueReceive(control_queue,&cmd,0)==pdTRUE) {
            if(cmd==0) {event=BikeCore_Key(&bike,now);if(event==2)sample(STORE_STOP,&sensor);if(event==1)last_sample=now;}
            else if(cmd==1)bike.view.screen=(uint8_t)((bike.view.screen+1)%3);
        }
        if(bike.view.state==BIKE_RIDE&&(uint32_t)(now-last_sample)>=10000) {
            last_sample=now;if(GpsFix_IsFresh(&bike.fix,now))sample(STORE_SAMPLE,&sensor);
        }
        snapshot.bike=bike.view;snapshot.fix=bike.fix;xQueueOverwrite(ui_queue,&snapshot);xQueueOverwrite(telemetry_queue,&snapshot);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,bike.view.state==BIKE_RIDE?GPIO_PIN_RESET:GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_1,bike.view.gps_valid?GPIO_PIN_RESET:GPIO_PIN_SET);
        app_health.control_cycles++;
        if((uint32_t)(now-last_health)>=1000) {health();last_health=now;}
        vTaskDelayUntil(&wake,pdMS_TO_TICKS(20));
    }
}
static int sendline(const char *s) {return App_DebugSend(s,(unsigned)strlen(s));}
static LegacyIndex legacy_index;
static StorageBackend backend,history_backend;
static int history_read(void *ctx,uint32_t address,uint8_t *p,unsigned n) {(void)ctx;return Board_FlashRead(address,p,n);}
static struct {uint8_t active,loaded;unsigned sample;uint32_t address,end;StorageSample items[13];unsigned count;RideSummary summary;} export_state;
static int begin_export(unsigned table) {
    uint32_t start,end;int rc=Legacy_ReadIndex(&history_backend,&legacy_index);
    if(rc)return rc;rc=Legacy_TableBounds(&legacy_index,table,&start,&end);if(rc)return rc;
    memset(&export_state,0,sizeof(export_state));export_state.active=1;export_state.address=start;export_state.end=end;return APP_OK;
}
static void export_tick(void) {
    char line[144];uint8_t page[256];int rc;
    if(!export_state.active)return;
    if(!export_state.loaded) {
        if(export_state.address>=export_state.end) {if(sendline("EXPORT END boundary\r\n")==APP_OK)export_state.active=0;return;}
        rc=Board_FlashRead(export_state.address,page,256);
        if(rc||StorageCodec_DecodeLegacy(page,export_state.items,&export_state.count,&export_state.summary)) {if(sendline("EXPORT END invalid/unwritten page\r\n")==APP_OK)export_state.active=0;return;}
        export_state.loaded=1;export_state.sample=0;
    }
    if(export_state.sample<export_state.count) {
        StorageSample *s=&export_state.items[export_state.sample];
        snprintf(line,sizeof(line),"PAGE %08lX SAMPLE %u lon_dm=%.8f lat_dm=%.8f speed=%.2f\r\n",(unsigned long)export_state.address,export_state.sample,s->longitude_dm,s->latitude_dm,s->speed_centi/100.0);
        if(sendline(line)==APP_OK)export_state.sample++;
    } else {snprintf(line,sizeof(line),"SUMMARY utc=%lu distance=%.2f max=%.2f avg=%.2f\r\n",(unsigned long)export_state.summary.timestamp,export_state.summary.distance/100.0,export_state.summary.max_speed/100.0,export_state.summary.average_speed/100.0);
        if(sendline(line)==APP_OK) {export_state.address+=256;export_state.loaded=0;}}
}
static void debug_command(const char *s) {
    uint8_t cmd=0;unsigned table;char line[144];int rc;uint8_t ee[4],reg=0;RestoreRequest restore;uint32_t start,end,created;
    if(!strcmp(s,"start stop")) {if(xQueueSend(control_queue,&cmd,0)!=pdTRUE)(void)sendline("CONTROL FULL\r\n");}
    else if(!strcmp(s,"screen")) {cmd=1;if(xQueueSend(control_queue,&cmd,0)!=pdTRUE)(void)sendline("CONTROL FULL\r\n");}
    else if(!strcmp(s,"clear data")||!strcmp(s,"normal mode")) (void)sendline("PROTECTED: historical writes disabled\r\n");
    else if(!strcmp(s,"get data")) {
        rc=Legacy_ReadIndex(&history_backend,&legacy_index);snprintf(line,sizeof(line),"INDEX result=%d count=%u version=%lu JEDEC=%06lX\r\n",rc,legacy_index.count,(unsigned long)legacy_index.version,(unsigned long)app_flash_jedec);(void)sendline(line);
    } else if(sscanf(s,"export %u",&table)==1) {
        rc=begin_export(table);if(rc) {snprintf(line,sizeof(line),"EXPORT error=%d\r\n",rc);(void)sendline(line);}
    } else if(!strcmp(s,"restore")) {
        rc=Legacy_ReadIndex(&history_backend,&legacy_index);
        if(!rc)rc=I2cBus_Transfer(&app_i2c,0x50,&reg,1,ee,4);
        if(!rc&&(ee[0]!=legacy_index.page||ee[1]==0xA5))rc=APP_INVALID;
        if(!rc)rc=Legacy_TableBounds(&legacy_index,legacy_index.count,&start,&end);
        if(!rc)rc=Storage_RestoreRead(&history_backend,start,end,(uint16_t)(ee[2]|((uint16_t)ee[3]<<8)),&restore.summary);
        created=legacy_index.created[legacy_index.next_table];
        /* Legacy timebase includes local offset; only differences are used here. */
        if(!rc&&(restore.summary.timestamp<created||(restore.summary.timestamp-created)>4294967U))rc=APP_INVALID;
        if(!rc) {restore.elapsed_ms=(restore.summary.timestamp-created)*1000U;if(xQueueSend(restore_queue,&restore,0)!=pdTRUE)rc=APP_FULL;}
        snprintf(line,sizeof(line),"RESTORE queued/result=%d; requires fresh GPS and IDLE; storage stays protected\r\n",rc);(void)sendline(line);
    } else if(!strcmp(s,"health")) {
        snprintf(line,sizeof(line),"HEAP free=%lu min=%lu STACK words=%lu/%lu/%lu GPS bad=%lu Qfull=%lu\r\n",(unsigned long)app_health.heap_free,(unsigned long)app_health.heap_min,(unsigned long)app_health.control_stack,(unsigned long)app_health.service_stack,(unsigned long)app_health.ui_stack,(unsigned long)app_health.gps_bad,(unsigned long)app_health.storage_queue_full);(void)sendline(line);
    } else (void)sendline("Commands: start stop, screen, get data, export N, restore, health\r\n");
}
static void ServiceTask(void *arg) {
    uint8_t bytes[64];size_t n,i;unsigned batch;uint32_t now,last_upload=0,last_time=0,last_nav=0,last_net_overflow=0,replan_deadline=0;Telemetry snapshot;
    NetTransport transport;StorageRequest request;StorageAck ack;char command[48];unsigned command_len=0;uint8_t dropping=0;
    char upload[512];NavView nav;NavigationCore navigation;uint32_t nav_version=0;
    (void)arg;memset(&navigation,0,sizeof(navigation));memset(&snapshot,0,sizeof(snapshot));history_backend.read=history_read;Board_StorageBackend(&backend);Storage_Init(&storage,&backend);
    memset(&transport,0,sizeof(transport));transport.send=App_NetSend;transport.enabled=APP_NETWORK_ENABLED;transport.host=APP_SERVER_HOST;transport.port=APP_SERVER_PORT;Network_Init(&network,&transport);
    if(APP_NETWORK_ENABLED)App_UartStart(2);
    for(;;) {
        now=HAL_GetTick();Network_Tick(&network,now);
        if(last_net_overflow!=app_health.network_rx_overflow) {
            last_net_overflow=app_health.network_rx_overflow;taskENTER_CRITICAL();(void)xStreamBufferReset(app_net_stream);taskEXIT_CRITICAL();Network_RxOverflow(&network,now);
        }
        for(batch=0;batch<4;batch++) {n=xStreamBufferReceive(app_net_stream,bytes,sizeof(bytes),0);for(i=0;i<n;i++)(void)Network_Byte(&network,bytes[i],now);if(n<sizeof(bytes))break;}
        n=xStreamBufferReceive(app_debug_stream,bytes,sizeof(bytes),0);
        for(i=0;i<n;i++) {
            if(bytes[i]=='\n') {if(!dropping) {command[command_len]=0;debug_command(command);}else (void)sendline("COMMAND TOO LONG\r\n");command_len=0;dropping=0;}
            else if(bytes[i]!='\r') {if(command_len<sizeof(command)-1&&!dropping)command[command_len++]=(char)bytes[i];else dropping=1;}
        }
        if(xQueueReceive(storage_queue,&request,0)==pdTRUE) {
            (void)Storage_Handle(&storage,&request,&ack);
            if(request.op==STORE_STOP||ack.result)configASSERT(xQueueSend(ack_queue,&ack,pdMS_TO_TICKS(20))==pdTRUE);
            app_health.storage_failures=storage.failures;app_health.flash_verify_failures=storage.verifies;
        }
        (void)xQueueReceive(telemetry_queue,&snapshot,0);
        if(network.state==NET_CONNECTED) {
            if(!last_time||(uint32_t)(now-last_time)>=180000) {const char *s="{\"cmd\":\"time request\",\"name\":\"dev_bike\"}";if(Network_Send(&network,s,(unsigned)strlen(s),now+1000,now)==APP_OK)last_time=now?now:1;}
            else if(snapshot.bike.state==BIKE_RIDE&&snapshot.bike.gps_valid&&network.has_time&&(uint32_t)(now-last_upload)>=2000) {
                int len=NetProtocol_Upload(upload,sizeof(upload),&snapshot.bike,&snapshot.fix,Network_Utc(&network,now));
                if(len>0&&Network_Send(&network,upload,(unsigned)len,now+1000,now)==APP_OK)last_upload=now;
            }
        }
        if(network.nav.version)NavigationCore_Tick(&navigation,&network.nav,&snapshot.bike);
        if(navigation.replan_pending==2&&(int32_t)(now-replan_deadline)>=0)navigation.replan_pending=1;
        if(NavigationCore_NeedsReplan(&navigation)&&network.state==NET_CONNECTED) {
            int len=snprintf(upload,sizeof(upload),"{\"cmd\":\"request_path_plan\",\"lat\":%.6f,\"lng\":%.6f}",network.nav.latitude,network.nav.longitude);
            if(len>0&&(unsigned)len<sizeof(upload)&&Network_Send(&network,upload,(unsigned)len,now+1000,now)==APP_OK) {navigation.replan_pending=2;replan_deadline=now+30000;}
        }
        if(network.nav.version&&((uint32_t)(now-last_nav)>=200||nav_version!=network.nav.version)) {
            nav_version=network.nav.version;
            memset(&nav,0,sizeof(nav));nav.step=network.nav.steps[navigation.step];nav.version=nav_version;nav.index=navigation.step;nav.count=network.nav.count;
            nav.remaining_m=navigation.remaining_mm/1000;xQueueOverwrite(nav_queue,&nav);last_nav=now;
        }
        export_tick();app_health.net_retries=network.retries;app_health.net_timeouts=network.timeouts;app_health.net_bad=network.malformed+network.framer.errors;app_health.service_cycles++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
static void UiTask(void *arg) {
    Telemetry snapshot;SensorSnapshot sensor;NavView nav;char lines[4][17];unsigned i;uint32_t now,next_oled=0;
    (void)arg;memset(&snapshot,0,sizeof(snapshot));memset(&sensor,0,sizeof(sensor));memset(&nav,0,sizeof(nav));
    for(;;) {
        now=HAL_GetTick();Board_Sensors(now,&sensor);Board_Magnetometer(&sensor);xQueueOverwrite(sensor_queue,&sensor);app_health.sensor_errors=sensor.errors;
        (void)xQueueReceive(ui_queue,&snapshot,0);(void)xQueueReceive(nav_queue,&nav,0);
        if((int32_t)(now-next_oled)>=0) {
            memset(lines,' ',sizeof(lines));for(i=0;i<4;i++)lines[i][16]=0;
            #if APP_STORAGE_WRITE_ENABLED
            snprintf(lines[0],17,"STATE %u GPS %u",(unsigned)snapshot.bike.state,(unsigned)snapshot.bike.gps_valid);
#else
            snprintf(lines[0],17,"S%u GPS%u STORE RO",(unsigned)snapshot.bike.state,(unsigned)snapshot.bike.gps_valid);
#endif
            if(snapshot.bike.screen==0) {
                snprintf(lines[1],17,"S %lu.%02lu km/h",(unsigned long)(snapshot.bike.speed_millikmh/1000),(unsigned long)(snapshot.bike.speed_millikmh/10%100));
                snprintf(lines[2],17,"D %lu.%02lu km",(unsigned long)(snapshot.bike.distance_mm/1000000),(unsigned long)(snapshot.bike.distance_mm/10000%100));
                snprintf(lines[3],17,"TIME %lu sec",(unsigned long)(snapshot.bike.elapsed_ms/1000));
            } else if(snapshot.bike.screen==1) {
                snprintf(lines[1],17,"AVG %lu.%02lu",(unsigned long)(snapshot.bike.average_speed_millikmh/1000),(unsigned long)(snapshot.bike.average_speed_millikmh/10%100));
                snprintf(lines[2],17,"MAX %lu K%lu",(unsigned long)(snapshot.bike.max_speed_millikmh/1000),(unsigned long)((uint64_t)snapshot.bike.distance_mm*35/1000000));
                if(sensor.aht_valid)snprintf(lines[3],17,"T %d H %u",sensor.temperature/100,sensor.humidity/100);else snprintf(lines[3],17,"SENSOR MISSING");
            } else {
                snprintf(lines[1],17,"STEP %u/%u %s",nav.index+1,nav.count,nav.step.instruction);
                snprintf(lines[2],17,"LEFT %lu m %s",(unsigned long)nav.remaining_m,nav.step.orientation);
                if(sensor.mag_valid)snprintf(lines[3],17,"HEADING %d",sensor.heading);else snprintf(lines[3],17,"MAG UNCONFIRMED");
            }
            if(snapshot.bike.storage_errors)snprintf(lines[3],17,"STORAGE ERROR");
            if(snapshot.bike.storage_overflows)snprintf(lines[3],17,"STORAGE FULL");
            if(snapshot.bike.state==BIKE_STOP_ERROR)snprintf(lines[3],17,"SAVE ERROR/LOCK");
            if(Board_Oled((const char (*)[17])lines)) {app_health.ui_errors++;next_oled=now+5000;}else next_oled=now+200;
        }
        app_health.ui_cycles++;vTaskDelay(pdMS_TO_TICKS(50));
    }
}
static void Heartbeat500(void *p) {TickType_t wake=xTaskGetTickCount();(void)p;for(;;){app_health.heartbeat_500++;vTaskDelayUntil(&wake,pdMS_TO_TICKS(500));}}
static void Heartbeat1000(void *p) {TickType_t wake=xTaskGetTickCount();(void)p;for(;;){app_health.heartbeat_1000++;health();vTaskDelayUntil(&wake,pdMS_TO_TICKS(1000));}}
void App_Start(void) {
#if APP_HEARTBEAT_ONLY
    configASSERT(xTaskCreate(Heartbeat500,"Beat500",192,0,2,&control_handle)==pdPASS);
    configASSERT(xTaskCreate(Heartbeat1000,"Beat1000",192,0,1,&service_handle)==pdPASS);
#else
    Board_Init();
    app_gps_stream=xStreamBufferCreate(APP_GPS_STREAM_SIZE,1);app_net_stream=xStreamBufferCreate(APP_NET_STREAM_SIZE,1);app_debug_stream=xStreamBufferCreate(64,1);
    configASSERT(app_gps_stream&&app_net_stream&&app_debug_stream);
    storage_queue=xQueueCreate(APP_STORAGE_QUEUE_DEPTH,sizeof(StorageRequest));ack_queue=xQueueCreate(2,sizeof(StorageAck));
    ui_queue=xQueueCreate(1,sizeof(Telemetry));telemetry_queue=xQueueCreate(1,sizeof(Telemetry));sensor_queue=xQueueCreate(1,sizeof(SensorSnapshot));nav_queue=xQueueCreate(1,sizeof(NavView));control_queue=xQueueCreate(4,1);restore_queue=xQueueCreate(1,sizeof(RestoreRequest));
    configASSERT(storage_queue&&ack_queue&&ui_queue&&telemetry_queue&&sensor_queue&&nav_queue&&control_queue&&restore_queue);
    configASSERT(xTaskCreate(ControlTask,"BikeGps",APP_CONTROL_STACK_WORDS,0,3,&control_handle)==pdPASS);
    configASSERT(xTaskCreate(ServiceTask,"StorageNet",APP_SERVICE_STACK_WORDS,0,2,&service_handle)==pdPASS);
    configASSERT(xTaskCreate(UiTask,"UiSensor",APP_UI_STACK_WORDS,0,1,&ui_handle)==pdPASS);
#endif
    vTaskStartScheduler();App_Assert("scheduler returned",0);
}
