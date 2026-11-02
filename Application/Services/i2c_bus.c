#include "i2c_bus.h"
static void delay(I2cBus *b) {b->delay(b->ctx);}
static int high(I2cBus *b) {
    unsigned n=100;b->scl(b->ctx,1);
    while(!b->read_scl(b->ctx)) {if(!n--) {b->timeouts++;return APP_TIMEOUT;}delay(b);}delay(b);return APP_OK;
}
static int start(I2cBus *b) {
    b->sda(b->ctx,1);delay(b);if(high(b))return APP_TIMEOUT;
    if(!b->read_sda(b->ctx)) {b->timeouts++;return APP_TIMEOUT;}
    b->sda(b->ctx,0);delay(b);b->scl(b->ctx,0);return APP_OK;
}
static int put(I2cBus *b,uint8_t byte) {
    unsigned i;int nack;
    for(i=0;i<8;i++) {b->sda(b->ctx,(byte&128)!=0);delay(b);if(high(b))return APP_TIMEOUT;b->scl(b->ctx,0);byte<<=1;}
    b->sda(b->ctx,1);delay(b);if(high(b))return APP_TIMEOUT;nack=b->read_sda(b->ctx);b->scl(b->ctx,0);
    if(nack) {b->nacks++;return APP_IO;}return APP_OK;
}
static int get(I2cBus *b,uint8_t *out,int last) {
    unsigned i;uint8_t v=0;b->sda(b->ctx,1);
    for(i=0;i<8;i++) {if(high(b))return APP_TIMEOUT;v=(uint8_t)((v<<1)|(b->read_sda(b->ctx)?1:0));b->scl(b->ctx,0);delay(b);}
    b->sda(b->ctx,last);delay(b);if(high(b))return APP_TIMEOUT;b->scl(b->ctx,0);b->sda(b->ctx,1);*out=v;return APP_OK;
}
int I2cBus_Transfer(I2cBus *b,uint8_t address,const uint8_t *tx,unsigned nt,uint8_t *rx,unsigned nr) {
    unsigned i;int rc;
    if(!b||address>127||(!tx&&nt)||(!rx&&nr))return APP_INVALID;
    if(b->acquire(b->ctx,10)!=APP_OK) {b->lock_failures++;return APP_TIMEOUT;}
    rc=start(b);if(rc)goto done;
    rc=put(b,(uint8_t)((address<<1)|(nt?0:1)));if(rc)goto done;
    for(i=0;i<nt;i++) {rc=put(b,tx[i]);if(rc)goto done;}
    if(nt&&nr) {rc=start(b);if(rc)goto done;rc=put(b,(uint8_t)((address<<1)|1));if(rc)goto done;}
    for(i=0;i<nr;i++) {rc=get(b,&rx[i],i+1==nr);if(rc)goto done;}
 done:
    b->sda(b->ctx,0);delay(b);if(high(b)&&rc==APP_OK)rc=APP_TIMEOUT;b->sda(b->ctx,1);delay(b);b->release(b->ctx);return rc;
}
