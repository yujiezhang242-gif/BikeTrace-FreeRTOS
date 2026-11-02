#ifndef APP_BOARD_H
#define APP_BOARD_H
#include "app_types.h"
#include "i2c_bus.h"
#include "storage_service.h"
extern I2cBus app_i2c;
typedef struct {int16_t temperature;uint16_t humidity;int16_t heading;uint8_t aht_valid,mag_valid;uint32_t errors,updated_ms;} SensorSnapshot;
void Board_Init(void);
void Board_Magnetometer(SensorSnapshot *s);
void Board_Sensors(uint32_t now,SensorSnapshot *s);
int Board_Oled(const char lines[4][17]);
void Board_StorageBackend(StorageBackend *io);
int Board_FlashRead(uint32_t address,uint8_t *out,unsigned length);
extern volatile uint32_t app_flash_jedec;
#endif
