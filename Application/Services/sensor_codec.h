#ifndef SENSOR_CODEC_H
#define SENSOR_CODEC_H
#include "app_types.h"
int Aht21_Decode(const uint8_t data[7],int16_t *temperature,uint16_t *humidity);
#endif
