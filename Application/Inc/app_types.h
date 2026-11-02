#ifndef APP_TYPES_H
#define APP_TYPES_H
#include <stdint.h>
#define GPS_STALE_MS 3000U
#define LEGACY_SIZE 256U
#define LEGACY_SAMPLES 13U
#define NET_MAX_FRAME 2048U
#define NAV_MAX_STEPS 16U
typedef struct {
    double latitude_deg, longitude_deg;
    uint32_t received_ms, utc_seconds, date_ddmmyy;
    uint32_t speed_milliknots;
    uint8_t valid;
} GpsFix;
typedef enum { BIKE_IDLE, BIKE_RIDE, BIKE_STOP_PENDING, BIKE_STOP_COMMITTED, BIKE_STOP_ERROR } BikeStatus;
typedef enum { NOT_RESTORE, IS_RESTORE } RestoreMode;
typedef struct {
    double longitude_dm, latitude_dm;
    uint32_t sample_id;
    uint16_t speed_centi;
} StorageSample;
typedef struct {
    uint32_t timestamp;
    uint16_t max_speed, average_speed;
    float kcal;
    uint16_t humidity;
    int16_t temperature;
    uint16_t distance;
} RideSummary;
typedef struct {
    BikeStatus state;
    uint32_t elapsed_ms, distance_mm, speed_millikmh, max_speed_millikmh;
    uint32_t average_speed_millikmh, stop_id, ride_id;
    uint32_t storage_errors, storage_overflows;
    int16_t temperature, average_temperature, heading;
    uint16_t humidity, average_humidity;
    uint8_t gps_valid, screen, sensor_valid, mag_valid;
} BikeSnapshot;
typedef enum { STORE_SAMPLE, STORE_STOP } StorageOp;
typedef struct {
    StorageOp op;
    uint32_t request_id, ride_id;
    StorageSample sample;
    RideSummary summary;
    uint8_t sample_valid;
} StorageRequest;
typedef struct { uint32_t request_id, address; int32_t result; } StorageAck;
enum { APP_OK=0, APP_INVALID=-1, APP_FULL=-2, APP_IO=-3,
       APP_PROTECTED=-4, APP_VERIFY=-5, APP_TIMEOUT=-6, APP_UNAVAILABLE=-7 };
int GpsFix_IsFresh(const GpsFix *fix, uint32_t now);
#endif
