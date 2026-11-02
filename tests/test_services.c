#include "gnss_parser.h"
#include "bike_core.h"
#include "storage_service.h"
#include "legacy_reader.h"
#include "network_service.h"
#include "i2c_bus.h"
#include "isolated_flash.h"
#include "navigation_core.h"
#include "sensor_codec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);abort();}}while(0)
static int feed(GnssParser *p,const char *body,uint32_t now,GpsFix *f) {
    char frame[300];unsigned i;unsigned char sum=0;int rc=0;
    for(i=0;body[i];i++)sum^=(unsigned char)body[i];
    snprintf(frame,sizeof(frame),"$%s*%02X\r\n",body,sum);
    for(i=0;frame[i];i++)rc=GnssParser_Feed(p,(uint8_t)frame[i],now,f);
    return rc;
}
static void gps_test(void) {
    GnssParser p;GpsFix f;unsigned i;char bad[301];GnssParser_Reset(&p);
    for(i=0;i<1000;i++)CHECK(feed(&p,"GPRMC,123519,A,4807.038,N,01131.000,E,10.0,084.4,230394,003.1,W",i*1000,&f)==1);
    CHECK(p.good==1000);CHECK(f.valid&&f.speed_milliknots==10000);CHECK(fabs(f.latitude_deg-48.1173)<1e-8);CHECK(f.utc_seconds==45319);
    CHECK(GpsFix_IsFresh(&f,999000+2999));CHECK(!GpsFix_IsFresh(&f,999000+3000));
    CHECK(feed(&p,"GNRMC,235959,A,2230.0,S,11330.0,W,1,0,291124,,,A",0xfffffff0U,&f)==1);
    CHECK(f.latitude_deg==-22.5&&f.longitude_deg==-113.5);CHECK(GpsFix_IsFresh(&f,100));
    CHECK(feed(&p,"GPRMC,123519,V,,,,,,,230394,,,N",1,&f)==1&&!f.valid);
    CHECK(feed(&p,"GPRMC,,V,,,,,,,,,N",1,&f)==1&&!f.valid);
    CHECK(feed(&p,"GPRMC,246060,A,4807.038,N,01131.000,E,10,0,230394,,,A",1,&f)==-1);
    CHECK(feed(&p,"GPRMC,123519,A,4860.000,N,01131.000,E,10,0,230394,,,A",1,&f)==-1);
    CHECK(feed(&p,"GPRMC,123519,A,4807.038,N,01131.000,E,NaN,0,230394,,,A",1,&f)==-1);
    CHECK(feed(&p,"GPRMC,123519,A,4807.038,N,01131.000,E,10,0,310224,,,A",1,&f)==-1);
    memset(bad,'a',300);bad[0]='$';bad[299]='\n';bad[300]=0;for(i=0;bad[i];i++)GnssParser_Feed(&p,(uint8_t)bad[i],0,&f);CHECK(p.oversized==1);
    for(i=0;i<sizeof("$GPRMC,x*00\r\n")-1;i++)GnssParser_Feed(&p,(uint8_t)"$GPRMC,x*00\r\n"[i],0,&f);CHECK(p.bad>=5);
    CHECK(feed(&p,"GPRMC,000000,A,0000,N,00000,E,0,0,010100,,,A",1,&f)==1);
    puts("T01/T02: 1000 RMC frames, checksum, CRLF, overflow, hemisphere, time/date, wrap PASS");
}
static void bike_test(void) {
    BikeCore b;GpsFix f;StorageAck a;StorageRequest r;RideSummary summary;unsigned i;
    memset(&f,0,sizeof(f));f.valid=1;f.speed_milliknots=10000;
    BikeCore_Init(&b,0);BikeCore_Gps(&b,&f);CHECK(BikeCore_Key(&b,0)==1);
    CHECK(BikeCore_Key(&b,20)==0);BikeCore_Tick(&b,2500);CHECK(b.view.elapsed_ms==2500);CHECK(b.view.speed_millikmh==18520);CHECK(b.view.distance_mm==12861);
    BikeCore_Tick(&b,10000);CHECK(b.view.elapsed_ms==10000);CHECK(b.view.distance_mm==15433);CHECK(!b.view.gps_valid);
    CHECK(BikeCore_Key(&b,10000)==2);BikeCore_MakeRequest(&b,&r,STORE_STOP,1);a.request_id=r.request_id+1;a.result=0;BikeCore_Ack(&b,&a);CHECK(b.view.state==BIKE_STOP_PENDING);
    a.request_id=r.request_id;a.result=APP_IO;BikeCore_Ack(&b,&a);CHECK(b.view.state==BIKE_STOP_ERROR);
    CHECK(BikeCore_Key(&b,10500)==2);a.request_id=b.view.stop_id;a.result=APP_OK;BikeCore_Ack(&b,&a);CHECK(b.view.state==BIKE_STOP_COMMITTED);BikeCore_Ack(&b,&a);CHECK(b.view.state==BIKE_STOP_COMMITTED);
    CHECK(BikeCore_Key(&b,11000)==1);CHECK(b.view.state==BIKE_IDLE);CHECK(BikeCore_Key(&b,12000)==0);
    BikeCore_Init(&b,0);b.view.state=BIKE_RIDE;b.view.distance_mm=1000000;b.view.elapsed_ms=600000;BikeCore_Tick(&b,0);CHECK(b.view.average_speed_millikmh==6000);
    BikeCore_Init(&b,0xffffff00U);f.received_ms=0xffffff00U;BikeCore_Gps(&b,&f);CHECK(BikeCore_Key(&b,0xffffff00U)==1);BikeCore_Tick(&b,244);CHECK(b.view.elapsed_ms==500);
    memset(&summary,0,sizeof(summary));summary.distance=100;BikeCore_Init(&b,100);f.received_ms=100;BikeCore_Gps(&b,&f);CHECK(BikeCore_Restore(&b,&summary,600000,100,IS_RESTORE)==0);CHECK(b.view.distance_mm==1000000);CHECK(BikeCore_Restore(&b,&summary,600000,100,IS_RESTORE)<0);
    /* Thirty minutes without network work: local time and fresh-fix distance continue. */
    BikeCore_Init(&b,0);f.received_ms=0;BikeCore_Gps(&b,&f);BikeCore_Key(&b,0);
    for(i=1;i<=1800;i++){BikeCore_Tick(&b,i*1000);f.received_ms=i*1000;BikeCore_Gps(&b,&f);}CHECK(b.view.elapsed_ms==1800000);CHECK(b.view.distance_mm==9260000);
    puts("T03/T04/T05: units, state/ACK/retry, elapsed delay, stale GPS, wrap, 30min offline simulation PASS");
}
static unsigned char flash[16384];static int fail_write,fail_check,corrupt_read;static unsigned writes,checkpoints;
static int rd(void *p,uint32_t a,uint8_t *s,unsigned n){(void)p;if(a+n>sizeof(flash))return APP_INVALID;memcpy(s,flash+a,n);if(corrupt_read&&writes&&n==256)s[0]^=1;return 0;}
static int wr(void *p,uint32_t a,const uint8_t *s,unsigned n){unsigned i;(void)p;if(fail_write)return APP_IO;if(a+n>sizeof(flash))return APP_INVALID;for(i=0;i<n;i++)flash[a+i]&=s[i];writes++;return 0;}
static int ck(void *p,uint32_t ride,uint32_t a,int stop){(void)p;(void)ride;(void)a;(void)stop;if(fail_check)return APP_IO;checkpoints++;return 0;}
static void storage_test(void) {
    StorageService s;StorageBackend io;StorageRequest r;StorageAck a;StorageSample items[13],decoded[13];RideSummary summary,out;uint8_t page[256];unsigned count,i;LegacyIndex idx;
    memset(items,0,sizeof(items));memset(&summary,0,sizeof(summary));summary.timestamp=0x12345678;summary.temperature=-1234;summary.distance=100;summary.kcal=35;summary.max_speed=1852;
    for(i=0;i<13;i++){items[i].longitude_dm=11330.0+i;items[i].latitude_dm=2230.0;items[i].speed_centi=1852;}
    CHECK(StorageCodec_EncodeLegacy(page,items,13,&summary)==0);CHECK(page[234]==0&&page[235]==0&&page[236]==0x78&&page[255]==0xa5);CHECK(page[16]==0x3c&&page[17]==7);CHECK(StorageCodec_DecodeLegacy(page,decoded,&count,&out)==0&&count==13);CHECK(out.temperature==-1234&&out.distance==100&&out.kcal==35);CHECK(decoded[12].longitude_dm==items[12].longitude_dm);
    {uint8_t golden[256];FILE *f=fopen("tests/golden/legacy_page.bin","rb");CHECK(f!=0);CHECK(fread(golden,1,256,f)==256);fclose(f);CHECK(!memcmp(page,golden,256));}
    page[255]=0;CHECK(StorageCodec_DecodeLegacy(page,decoded,&count,&out)==APP_INVALID);
    memset(&io,0,sizeof(io));io.read=rd;io.program=wr;io.checkpoint=ck;io.start=8192;io.end=16384;io.writes_enabled=1;
    memset(flash,255,sizeof(flash));writes=checkpoints=0;Storage_Init(&s,&io);memset(&r,0,sizeof(r));r.ride_id=1;r.sample_valid=1;r.summary=summary;r.sample=items[0];
    for(i=0;i<13;i++){r.sample.sample_id=i;CHECK(Storage_Handle(&s,&r,&a)==0);}CHECK(writes==1&&s.pages==1&&s.count==0);
    r.op=STORE_STOP;r.request_id=9;CHECK(Storage_Handle(&s,&r,&a)==0);CHECK(s.committed);i=writes;CHECK(Storage_Handle(&s,&r,&a)==0&&writes==i);
    memset(flash,255,sizeof(flash));writes=0;Storage_Init(&s,&io);r.op=STORE_SAMPLE;r.ride_id=2;r.sample.sample_id=1;CHECK(Storage_Handle(&s,&r,&a)==0);r.sample.sample_id=2;CHECK(Storage_Handle(&s,&r,&a)==0);
    r.op=STORE_STOP;r.request_id=10;fail_write=1;CHECK(Storage_Handle(&s,&r,&a)==APP_IO&&s.count==2&&!s.committed);fail_write=0;fail_check=1;CHECK(Storage_Handle(&s,&r,&a)==APP_IO&&s.programmed&&s.count==2);fail_check=0;r.request_id=11;CHECK(Storage_Handle(&s,&r,&a)==0&&s.count==0&&s.pages==1);CHECK(writes==1);
    CHECK(Storage_RestoreRead(&io,8192,16384,1,&out)==0&&out.distance==100);CHECK(Storage_RestoreRead(&io,8192,16384,0,&out)==APP_INVALID);CHECK(Storage_RestoreRead(&io,8192,16384,65535,&out)==APP_INVALID);CHECK(Storage_RestoreRead(&io,8192,8448,2,&out)==APP_INVALID);
    io.writes_enabled=0;Storage_Init(&s,&io);CHECK(Storage_Handle(&s,&r,&a)==APP_PROTECTED&&s.count>0);io.writes_enabled=1;
    memset(flash,255,sizeof(flash));writes=0;Storage_Init(&s,&io);corrupt_read=1;CHECK(Storage_Handle(&s,&r,&a)==APP_VERIFY&&s.count>0);corrupt_read=0;
    memset(flash,255,sizeof(flash));CHECK(Legacy_ReadIndex(&io,&idx)==APP_UNAVAILABLE);
    /* Valid page zero must not be confused with erased factory state. */
    memset(flash,0,76);flash[0]=1;flash[75]=0xa5;CHECK(Legacy_ReadIndex(&io,&idx)==0&&idx.count==1);{uint32_t start,end;CHECK(Legacy_TableBounds(&idx,1,&start,&end)==0&&start==8192&&end==0x21000);CHECK(Legacy_TableBounds(&idx,0,&start,&end)==APP_INVALID);}
    flash[2]=255;CHECK(Legacy_ReadIndex(&io,&idx)==APP_INVALID);
    puts("T06/T07/T08: legacy offsets, 13 samples, STOP, write/readback/checkpoint failure, retries, protection, page-zero index PASS");
}
static uint8_t isolated_memory[65536];static unsigned isolated_calls;static int cut_after=-1;
static int iso_read(void *p,uint32_t a,uint8_t *out,unsigned n) {(void)p;if(a<0x200000||a>0x210000||n>0x210000-a)return APP_INVALID;memcpy(out,isolated_memory+(a-0x200000),n);return 0;}
static int iso_write(void *p,uint32_t a,const uint8_t *in,unsigned n) {
    unsigned i;(void)p;if(cut_after==0)return APP_IO;if(cut_after>0)cut_after--;
    if(a<0x200000||a>0x210000||n>0x210000-a)return APP_INVALID;
    for(i=0;i<n;i++)isolated_memory[a-0x200000+i]&=in[i];isolated_calls++;return 0;
}
static void isolated_test(void) {
    IsolatedFlash f,reboot;IsolatedFlashConfig c;StorageBackend io;StorageService s;StorageRequest r;StorageAck ack;unsigned i;uint8_t copy[65536];
    memset(&c,0,sizeof(c));c.read=iso_read;c.program=iso_write;c.capacity=0x210000;c.start=0x200000;c.end=0x210000;
    CHECK(IsolatedFlash_Init(&f,&c,&io)==APP_PROTECTED);c.approved=1;c.start=0x100000;CHECK(IsolatedFlash_Init(&f,&c,&io)==APP_PROTECTED);c.start=0x200000;
    memset(isolated_memory,255,sizeof(isolated_memory));CHECK(IsolatedFlash_Init(&f,&c,&io)==0);Storage_Init(&s,&io);
    memset(&r,0,sizeof(r));r.ride_id=1;r.op=STORE_STOP;r.request_id=1;r.sample_valid=1;r.sample.latitude_dm=2230;r.sample.longitude_dm=11330;
    /* Cut after data and journal payload, before checkpoint commit marker. */
    cut_after=2;CHECK(Storage_Handle(&s,&r,&ack)==APP_IO&&s.count==1);cut_after=-1;
    CHECK(IsolatedFlash_Init(&reboot,&c,&io)==0&&!reboot.found&&reboot.next_slot==1);
    CHECK(Storage_Handle(&s,&r,&ack)==0&&f.found&&f.last_stop&&s.pages==1);
    memcpy(copy,isolated_memory,sizeof(copy));
    for(i=0;i<20;i++){CHECK(IsolatedFlash_Init(&reboot,&c,&io)==0&&reboot.found&&reboot.last_address==0x200000);CHECK(!memcmp(copy,isolated_memory,sizeof(copy)));}
    /* Exhaust journal without erasing or overwriting any old record. */
    Storage_Init(&s,&io);
    for(i=2;i<=127;i++){r.ride_id=i;r.request_id=i;r.sample.sample_id=i;CHECK(Storage_Handle(&s,&r,&ack)==0);}
    r.ride_id=128;r.request_id=128;CHECK(Storage_Handle(&s,&r,&ack)==APP_FULL&&s.count==1);
    CHECK(isolated_calls>0);puts("T07/T08 extended: isolated range guard, interrupted checkpoint, 20 read-only reboot scans, journal full PASS (RAM fake)");
}
static int sent;static char last_send[160];
static int send_mock(void *p,const char *s,unsigned n){(void)p;CHECK(n<sizeof(last_send));memcpy(last_send,s,n);last_send[n]=0;sent++;return 0;}
static void net_test(void) {
    NetFramer f;JsonWorkspace w;Navigation nav,saved;uint32_t utc=0;unsigned i,j,frames=0;NetworkService n;NetTransport io;char upload[512];BikeSnapshot b;GpsFix fix;
    const char *msg="{\"cmd\":\"time response\",\"time\":123}";
    const char *path="{\"cmd\":\"path_plan\",\"data\":{\"paths\":{\"distance\":100,\"duration\":20,\"dest_lat\":22.5,\"dest_lng\":113.5,\"steps\":[{\"inst\":\"L\",\"orie\":\"NE\",\"dist\":100}]}}}";
    memset(&nav,0,sizeof(nav));NetFramer_Init(&f);
    for(j=0;j<10;j++)for(i=0;msg[i];i++)if(NetFramer_Feed(&f,(uint8_t)msg[i])==1){frames++;CHECK(NetProtocol_Parse(&w,f.data,f.length,&utc,&nav)==1);}CHECK(frames==10&&utc==123);
    CHECK(NetProtocol_Parse(&w,path,(unsigned)strlen(path),&utc,&nav)==2&&nav.count==1&&nav.steps[0].distance==100);saved=nav;
    {const char *bad[]={"{\"cmd\":\"time response\",\"time\":01}","{\"cmd\":\"time response\",\"time\":-1}","{\"cmd\":\"time response\",\"time\":1.5}","{\"cmd\":\"time response\",\"time\":NaN}","{\"cmd\":\"time response\",\"time\":1,}","{\"cmd\":\"time response\",\"cmd\":\"time response\",\"time\":1}","{} trailing","{\"cmd\":\"path_plan\",\"data\":{}}","{\"a\":\"\\x\"}","{\"a\":[}}"};for(i=0;i<sizeof(bad)/sizeof(bad[0]);i++)CHECK(NetProtocol_Parse(&w,bad[i],(unsigned)strlen(bad[i]),&utc,&nav)<0);}
    CHECK(memcmp(&nav,&saved,sizeof(nav))==0);
    {char many[2049];unsigned k;for(k=16;k<=17;k++) {unsigned pos=(unsigned)snprintf(many,sizeof(many),"{\"cmd\":\"path_plan\",\"data\":{\"paths\":{\"distance\":100,\"duration\":20,\"dest_lat\":22.5,\"dest_lng\":113.5,\"steps\":[");
        for(j=0;j<k;j++)pos+=(unsigned)snprintf(many+pos,sizeof(many)-pos,"%s{\"inst\":\"L\",\"orie\":\"NE\",\"dist\":1}",j?",":"");
        pos+=(unsigned)snprintf(many+pos,sizeof(many)-pos,"]}}}");CHECK(NetProtocol_Parse(&w,many,pos,&utc,&nav)==(k==16?2:APP_INVALID));}}

    NetFramer_Init(&f);NetFramer_Feed(&f,'{');for(i=0;i<2100;i++)NetFramer_Feed(&f,' ');CHECK(NetFramer_Feed(&f,'}')==-1&&f.oversized==1);
    frames=0;for(i=0;msg[i];i++)if(NetFramer_Feed(&f,(uint8_t)msg[i])==1)frames++;CHECK(frames==1);
    memset(&io,0,sizeof(io));io.send=send_mock;io.host="192.0.2.1";io.port=8081;io.enabled=1;Network_Init(&n,&io);Network_Tick(&n,0);CHECK(n.waiting&&!strcmp(last_send,"AT\r"));Network_Tick(&n,3000);CHECK(n.state==NET_BACKOFF&&n.timeouts==1);Network_Tick(&n,4000);CHECK(n.waiting);
    {const char *reply="OK\r\n";for(i=0;reply[i];i++)Network_Byte(&n,(uint8_t)reply[i],4010);CHECK(n.command==1);}
    n.state=NET_CONNECTED;n.waiting=0;for(i=0;msg[i];i++)Network_Byte(&n,(uint8_t)msg[i],5000);CHECK(Network_Utc(&n,7000)==125);
    {const char *closed="\r\nNO CARRIER\r\n";for(i=0;closed[i];i++)Network_Byte(&n,(uint8_t)closed[i],8000);CHECK(n.state==NET_BACKOFF);}
    memset(&b,0,sizeof(b));memset(&fix,0,sizeof(fix));CHECK(NetProtocol_Upload(upload,sizeof(upload),&b,&fix,1)>0);CHECK(NetProtocol_Upload(upload,8,&b,&fix,1)==APP_FULL);
    /* Deterministic malformed byte pressure, ASan/UBSan guard all parsers. */
    srand(7);for(j=0;j<5000;j++){unsigned len=(unsigned)(rand()%511);for(i=0;i<len;i++)upload[i]=(char)(rand()%128);upload[len]=0;(void)NetProtocol_Parse(&w,upload,len,&utc,&nav);for(i=0;i<len;i++)NetFramer_Feed(&f,(uint8_t)upload[i]);}
    puts("T09/T10: split/coalesced JSON, bounded parser, malformed fuzz, atomic routes, time, AT timeout/URC/backoff PASS");
}
static void navigation_test(void) {
    Navigation p;NavigationCore n;BikeSnapshot b;
    memset(&p,0,sizeof(p));memset(&n,0,sizeof(n));memset(&b,0,sizeof(b));p.version=1;p.count=2;p.steps[0].distance=100;p.steps[1].distance=50;strcpy(p.steps[0].orientation,"N");strcpy(p.steps[1].orientation,"E");b.state=BIKE_RIDE;b.ride_id=1;
    NavigationCore_Tick(&n,&p,&b);b.distance_mm=120000;NavigationCore_Tick(&n,&p,&b);CHECK(n.step==1&&n.remaining_mm==30000);
    b.distance_mm=150000;NavigationCore_Tick(&n,&p,&b);CHECK(n.done&&n.remaining_mm==0);
    b.ride_id=2;b.distance_mm=0;b.mag_valid=1;b.heading=180;NavigationCore_Tick(&n,&p,&b);b.distance_mm=100000;NavigationCore_Tick(&n,&p,&b);CHECK(NavigationCore_NeedsReplan(&n));CHECK(n.remaining_mm==200000);
    puts("Navigation: multiple-step advance, endpoint, new ride reset, heading deviation/replan PASS");
}
static void sensor_test(void) {
    uint8_t d[7]={8,0x80,0,6,0,0,0};uint8_t crc=255;unsigned i,j;int16_t temperature=0;uint16_t humidity=0;
    for(i=0;i<6;i++){crc^=d[i];for(j=0;j<8;j++)crc=(uint8_t)((crc&128)?(crc<<1)^0x31:crc<<1);}d[6]=crc;
    CHECK(Aht21_Decode(d,&temperature,&humidity)==0&&temperature==2500&&humidity==5000);
    d[6]^=1;CHECK(Aht21_Decode(d,&temperature,&humidity)==APP_VERIFY);d[0]=0x88;CHECK(Aht21_Decode(d,&temperature,&humidity)==APP_TIMEOUT);
    puts("AHT21: CRC, busy-state rejection, 25.00C/50.00% decode PASS");
}
static int locked,released,sda_state=1,scl_state=1,stuck,nack;
static int acquire(void *p,uint32_t t){(void)p;(void)t;locked++;return 0;}
static void release(void *p){(void)p;released++;}
static void scl(void *p,int v){(void)p;scl_state=v;}
static void sda(void *p,int v){(void)p;sda_state=v;}
static int read_scl(void *p){(void)p;return stuck?0:scl_state;}
static int read_sda(void *p){(void)p;return nack?sda_state:0;}
static void pause_bus(void *p){(void)p;}
static unsigned bus_read_index;
static int read_script(void *p) {
    static const uint8_t levels[]={1,0,0,1,0,1,0,1,0,1,0,1,0};(void)p;
    CHECK(bus_read_index<sizeof(levels));return levels[bus_read_index++];
}
static void bus_test(void) {
    I2cBus b;uint8_t tx=0;memset(&b,0,sizeof(b));b.acquire=acquire;b.release=release;b.scl=scl;b.sda=sda;b.read_scl=read_scl;b.read_sda=read_sda;b.delay=pause_bus;
    nack=1;CHECK(I2cBus_Transfer(&b,0x3c,&tx,1,0,0)==APP_IO);CHECK(locked==released&&sda_state==1);
    stuck=1;CHECK(I2cBus_Transfer(&b,0x3c,&tx,1,0,0)==APP_TIMEOUT);CHECK(locked==released&&sda_state==1&&b.timeouts>0);stuck=0;
    CHECK(I2cBus_Transfer(&b,0xff,&tx,1,0,0)==APP_INVALID);CHECK(locked==released);
    {uint8_t rx=0;b.read_sda=read_script;CHECK(I2cBus_Transfer(&b,0x50,&tx,1,&rx,1)==APP_OK);CHECK(rx==0xaa&&bus_read_index==13);CHECK(locked==released);}
    puts("T11: repeated START/read bits, missing device NACK, stuck SCL timeout, STOP/release PASS");
}
int main(void){gps_test();bike_test();storage_test();isolated_test();net_test();navigation_test();sensor_test();bus_test();printf("PASS %u checks; sizeof GpsFix=%zu Snapshot=%zu StorageRequest=%zu\n",checks,sizeof(GpsFix),sizeof(BikeSnapshot),sizeof(StorageRequest));return 0;}
