#include "bike_core.h"
#include <string.h>
#include <limits.h>
static uint16_t centi(uint32_t milli) { uint32_t v=(milli+5)/10;return (uint16_t)(v>65535?65535:v); }
void BikeCore_Init(BikeCore *b,uint32_t now) { memset(b,0,sizeof(*b));b->last_ms=now; }
void BikeCore_Sensor(BikeCore *b,int16_t temperature,uint16_t humidity,int valid) {
    b->view.sensor_valid=(uint8_t)(valid!=0);if(!valid)return;
    b->view.temperature=temperature;b->view.humidity=humidity;
    if(b->view.state==BIKE_RIDE) {
        b->temperature_sum+=temperature;b->humidity_sum+=humidity;b->sensor_count++;
        b->view.average_temperature=(int16_t)(b->temperature_sum/b->sensor_count);
        b->view.average_humidity=(uint16_t)(b->humidity_sum/b->sensor_count);
    }
}
void BikeCore_Gps(BikeCore *b,const GpsFix *f) { b->fix=*f;b->view.gps_valid=(uint8_t)GpsFix_IsFresh(f,b->last_ms);b->view.speed_millikmh=b->view.gps_valid?(uint32_t)(((uint64_t)f->speed_milliknots*1852+500)/1000):0; }
void BikeCore_Tick(BikeCore *b,uint32_t now) {
    uint32_t dt=now-b->last_ms, fresh_ms=0, age; uint64_t distance;
    age=b->last_ms-b->fix.received_ms;
    if(b->fix.valid&&age<GPS_STALE_MS)fresh_ms=GPS_STALE_MS-age;
    if(fresh_ms>dt)fresh_ms=dt;
    b->last_ms=now;b->view.gps_valid=(uint8_t)GpsFix_IsFresh(&b->fix,now);
    b->view.speed_millikmh=b->view.gps_valid?(uint32_t)(((uint64_t)b->fix.speed_milliknots*1852+500)/1000):0;
    if(b->view.state!=BIKE_RIDE)return;
    if(UINT32_MAX-b->view.elapsed_ms<dt) b->view.elapsed_ms=UINT32_MAX; else b->view.elapsed_ms+=dt;
    /* milli-knots * 1.852 = milli-km/h; mm = milli-km/h * ms / 3600. */
    distance=(uint64_t)(b->fix.valid?((uint64_t)b->fix.speed_milliknots*1852+500)/1000:0)*fresh_ms+b->distance_remainder;
    b->distance_remainder=distance%3600;distance/=3600;
    if(distance>UINT32_MAX-b->view.distance_mm)b->view.distance_mm=UINT32_MAX;else b->view.distance_mm+=(uint32_t)distance;
    if(b->view.speed_millikmh>b->view.max_speed_millikmh)b->view.max_speed_millikmh=b->view.speed_millikmh;
    b->view.average_speed_millikmh=b->view.elapsed_ms?(uint32_t)((uint64_t)b->view.distance_mm*3600/b->view.elapsed_ms):0;
}
int BikeCore_Key(BikeCore *b,uint32_t now) {
    uint32_t ride;
    if(b->key_seen&&(uint32_t)(now-b->last_key_ms)<250)return 0;
    b->key_seen=1;b->last_key_ms=now;BikeCore_Tick(b,now);
    switch(b->view.state) {
    case BIKE_IDLE:
        if(!GpsFix_IsFresh(&b->fix,now))return 0;
        ride=b->view.ride_id+1;memset(&b->view,0,sizeof(b->view));b->view.ride_id=ride;
        b->view.state=BIKE_RIDE;b->view.gps_valid=1;b->distance_remainder=0;b->temperature_sum=0;b->humidity_sum=0;b->sensor_count=0;return 1;
    case BIKE_RIDE: case BIKE_STOP_ERROR:
        b->view.state=BIKE_STOP_PENDING;b->view.stop_id++;if(!b->view.stop_id)b->view.stop_id++;
        return 2;
    case BIKE_STOP_COMMITTED:b->view.state=BIKE_IDLE;return 1;
    default:return 0;
    }
}
void BikeCore_Ack(BikeCore *b,const StorageAck *a) {
    if(b->view.state!=BIKE_STOP_PENDING||a->request_id!=b->view.stop_id)return;
    b->view.state=a->result==APP_OK?BIKE_STOP_COMMITTED:BIKE_STOP_ERROR;
    if(a->result)b->view.storage_errors++;
}
int BikeCore_Restore(BikeCore *b,const RideSummary *s,uint32_t elapsed,uint32_t now,RestoreMode mode) {
    if(mode!=IS_RESTORE||b->view.state!=BIKE_IDLE||!GpsFix_IsFresh(&b->fix,now))return APP_INVALID;
    b->view.distance_mm=(uint32_t)s->distance*10000;b->view.elapsed_ms=elapsed;
    b->view.max_speed_millikmh=(uint32_t)s->max_speed*10;b->view.average_speed_millikmh=(uint32_t)s->average_speed*10;
    b->view.state=BIKE_RIDE;b->view.ride_id++;b->last_ms=now;return APP_OK;
}
static double dm(double deg) { int d=(int)deg;return d*100+(deg-d)*60; }
void BikeCore_MakeRequest(BikeCore *b,StorageRequest *r,StorageOp op,uint32_t timestamp) {
    memset(r,0,sizeof(*r));r->sample_valid=(uint8_t)GpsFix_IsFresh(&b->fix,b->last_ms);r->op=op;r->ride_id=b->view.ride_id;r->request_id=op==STORE_STOP?b->view.stop_id:0;
    /* Same GPS fix at STOP uses same sample id for deduplication. */
    r->sample.sample_id=b->fix.received_ms;
    r->sample.latitude_dm=dm(b->fix.latitude_deg);r->sample.longitude_dm=dm(b->fix.longitude_deg);
    r->sample.speed_centi=centi(b->view.speed_millikmh);
    r->summary.timestamp=timestamp;r->summary.max_speed=centi(b->view.max_speed_millikmh);
    r->summary.average_speed=centi(b->view.average_speed_millikmh);
    r->summary.distance=(uint16_t)(b->view.distance_mm/10000>65535?65535:b->view.distance_mm/10000);
    r->summary.temperature=b->view.average_temperature;r->summary.humidity=b->view.average_humidity;
    r->summary.kcal=(float)b->view.distance_mm/1000000.0f*35.0f;
}
