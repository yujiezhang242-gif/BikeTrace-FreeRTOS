#ifndef I2C_BUS_H
#define I2C_BUS_H
#include "app_types.h"
typedef struct {
    void *ctx;
    int (*acquire)(void *,uint32_t);
    void (*release)(void *);
    void (*scl)(void *,int);
    void (*sda)(void *,int);
    int (*read_scl)(void *);
    int (*read_sda)(void *);
    void (*delay)(void *);
    uint32_t nacks,timeouts,lock_failures;
} I2cBus;
/* 7-bit address. Write followed by repeated START read. Every exit releases bus. */
int I2cBus_Transfer(I2cBus *b,uint8_t address,const uint8_t *tx,unsigned nt,uint8_t *rx,unsigned nr);
#endif
