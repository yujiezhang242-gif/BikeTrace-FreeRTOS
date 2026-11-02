#ifndef BIKE_CORE_H
#define BIKE_CORE_H
#include "app_types.h"
typedef struct {
    BikeSnapshot view;
    GpsFix fix;
    uint32_t last_ms, last_key_ms, sample_id;
    uint64_t distance_remainder;
    int64_t temperature_sum;
    uint64_t humidity_sum;
    uint32_t sensor_count;
    uint8_t key_seen;
} BikeCore;
void BikeCore_Init(BikeCore *b,uint32_t now);
void BikeCore_Gps(BikeCore *b,const GpsFix *fix);
void BikeCore_Sensor(BikeCore *b,int16_t temperature,uint16_t humidity,int valid);
void BikeCore_Tick(BikeCore *b,uint32_t now);
int BikeCore_Key(BikeCore *b,uint32_t now);
void BikeCore_Ack(BikeCore *b,const StorageAck *ack);
int BikeCore_Restore(BikeCore *b,const RideSummary *s,uint32_t elapsed_ms,uint32_t now,RestoreMode mode);
void BikeCore_MakeRequest(BikeCore *b,StorageRequest *r,StorageOp op,uint32_t timestamp);
#endif
