#include "sensor_codec.h"
int Aht21_Decode(const uint8_t *rx,int16_t *temperature,uint16_t *humidity) {
    unsigned i,j;uint8_t crc=0xFF;uint32_t rh,t;
    if(!rx||!temperature||!humidity)return APP_INVALID;
    if(rx[0]&0x80)return APP_TIMEOUT;if(!(rx[0]&0x08))return APP_INVALID;
    for(i=0;i<6;i++) {crc^=rx[i];for(j=0;j<8;j++)crc=(uint8_t)((crc&0x80)?(crc<<1)^0x31:crc<<1);}
    if(crc!=rx[6])return APP_VERIFY;
    rh=((uint32_t)rx[1]<<12)|((uint32_t)rx[2]<<4)|(rx[3]>>4);t=((uint32_t)(rx[3]&15)<<16)|((uint32_t)rx[4]<<8)|rx[5];
    *humidity=(uint16_t)((uint64_t)rh*10000/1048576);*temperature=(int16_t)((int32_t)((uint64_t)t*20000/1048576)-5000);return APP_OK;
}
