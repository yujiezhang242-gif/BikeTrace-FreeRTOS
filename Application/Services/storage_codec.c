#include "storage_codec.h"
#include <string.h>
#include <float.h>
/* memcpy avoids unaligned loads. Byte reversal explicitly supports big-endian hosts. */
static int little(void) { const uint16_t n=1;return *(const uint8_t *)&n; }
static void bytes(uint8_t *d,const void *v,unsigned n) { const uint8_t *s=(const uint8_t *)v;unsigned i;for(i=0;i<n;i++)d[i]=s[little()?i:n-1-i]; }
static void value(void *d,const uint8_t *s,unsigned n) { uint8_t *p=(uint8_t *)d;unsigned i;for(i=0;i<n;i++)p[little()?i:n-1-i]=s[i]; }
static void put16(uint8_t *p,uint16_t v) {p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint16_t get16(const uint8_t *p) {return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static void put32(uint8_t *p,uint32_t v) {unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t get32(const uint8_t *p) {return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int abi(void) {return sizeof(double)==8&&sizeof(float)==4&&DBL_MANT_DIG==53&&FLT_MANT_DIG==24;}
int StorageCodec_EncodeLegacy(uint8_t *p,const StorageSample *s,unsigned n,const RideSummary *r) {
    unsigned i;if(!p||!s||!r||n>13||!abi())return APP_INVALID;
    memset(p,0,256);
    for(i=0;i<n;i++) { bytes(p+i*18,&s[i].longitude_dm,8);bytes(p+i*18+8,&s[i].latitude_dm,8);put16(p+i*18+16,s[i].speed_centi); }
    put32(p+236,r->timestamp);put16(p+240,r->max_speed);put16(p+242,r->average_speed);
    bytes(p+244,&r->kcal,4);put16(p+248,r->humidity);put16(p+250,(uint16_t)r->temperature);put16(p+252,r->distance);p[255]=0xA5;
    return APP_OK;
}
int StorageCodec_DecodeLegacy(const uint8_t *p,StorageSample *s,unsigned *n,RideSummary *r) {
    unsigned i,j;int zero;
    if(!p||!s||!n||!r||p[255]!=0xA5||!abi())return APP_INVALID;
    *n=0;memset(s,0,13*sizeof(*s));memset(r,0,sizeof(*r));
    for(i=0;i<13;i++) {zero=1;for(j=0;j<18;j++)if(p[i*18+j])zero=0;if(zero)break;
        value(&s[i].longitude_dm,p+i*18,8);value(&s[i].latitude_dm,p+i*18+8,8);s[i].speed_centi=get16(p+i*18+16);(*n)++;
        if(!(s[i].longitude_dm>=-18000&&s[i].longitude_dm<=18000&&s[i].latitude_dm>=-9000&&s[i].latitude_dm<=9000))return APP_INVALID;
    }
    r->timestamp=get32(p+236);r->max_speed=get16(p+240);r->average_speed=get16(p+242);
    value(&r->kcal,p+244,4);r->humidity=get16(p+248);r->temperature=(int16_t)get16(p+250);r->distance=get16(p+252);
    if(!(r->kcal>=0&&r->kcal<1e9f))return APP_INVALID;
    return APP_OK;
}
