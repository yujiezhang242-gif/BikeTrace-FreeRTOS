#include "app_board.h"
#include "app_config.h"
#include "main.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "font8x16.h"
#include "isolated_flash.h"
#include "sensor_codec.h"
#include <string.h>
#include <math.h>
static SemaphoreHandle_t mutex;
I2cBus app_i2c;
volatile uint32_t app_flash_jedec;
static int lock(void *p,uint32_t ms) {(void)p;return xSemaphoreTake(mutex,pdMS_TO_TICKS(ms))==pdTRUE?APP_OK:APP_TIMEOUT;}
static void unlock(void *p) {(void)p;xSemaphoreGive(mutex);}
static void scl(void *p,int high) {(void)p;HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,high?GPIO_PIN_SET:GPIO_PIN_RESET);}
static void sda(void *p,int high) {(void)p;HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,high?GPIO_PIN_SET:GPIO_PIN_RESET);}
static int rscl(void *p) {(void)p;return HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_6)==GPIO_PIN_SET;}
static int rsda(void *p) {(void)p;return HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_7)==GPIO_PIN_SET;}
static void short_delay(void *p) {volatile unsigned n;(void)p;for(n=0;n<100;n++)__NOP();}
void Board_Init(void) {
    GPIO_InitTypeDef pins;memset(&pins,0,sizeof(pins));__HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6|GPIO_PIN_7,GPIO_PIN_SET);pins.Pin=GPIO_PIN_6|GPIO_PIN_7;pins.Mode=GPIO_MODE_OUTPUT_OD;pins.Speed=GPIO_SPEED_FREQ_HIGH;HAL_GPIO_Init(GPIOB,&pins);
    mutex=xSemaphoreCreateMutex();configASSERT(mutex);
    memset(&app_i2c,0,sizeof(app_i2c));app_i2c.acquire=lock;app_i2c.release=unlock;app_i2c.scl=scl;app_i2c.sda=sda;app_i2c.read_scl=rscl;app_i2c.read_sda=rsda;app_i2c.delay=short_delay;
}
static int command(uint8_t c) {uint8_t tx[2]={0,c};return I2cBus_Transfer(&app_i2c,APP_OLED_ADDRESS,tx,2,0,0);}
int Board_Oled(const char lines[4][17]) {
    static uint8_t initialized;
    static const uint8_t init[]={0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0,0x40,0xA1,0xC8,0xDA,0x12,0x81,0xCF,0xD9,0xF1,0xDB,0x30,0xA4,0xA6,0x8D,0x14,0xAF};
    uint8_t tx[129];unsigned p,x,j;int rc;
    if(!initialized) {for(j=0;j<sizeof(init);j++) {rc=command(init[j]);if(rc)return rc;}initialized=1;}
    for(p=0;p<8;p++) {
        if(command((uint8_t)(0xB0+p))||command(0)||command(0x10)) {initialized=0;return APP_IO;}
        tx[0]=0x40;
        for(x=0;x<16;x++) {unsigned char c=(unsigned char)lines[p/2][x];if(c<32||c>126)c=32;memcpy(tx+1+x*8,OLED_F8x16[c-32]+(p%2)*8,8);}
        rc=I2cBus_Transfer(&app_i2c,APP_OLED_ADDRESS,tx,129,0,0);if(rc) {initialized=0;return rc;}
    }return APP_OK;
}
void Board_Sensors(uint32_t now,SensorSnapshot *s) {
    static uint8_t pending,init_tries;static uint32_t deadline,next=40;
    uint8_t tx[3]={0xAC,0x33,0},rx[7],status_cmd=0x71,status;
    if(!pending) {
        if((int32_t)(now-next)<0)return;
        next=now+2000;
        if(I2cBus_Transfer(&app_i2c,APP_AHT_ADDRESS,&status_cmd,1,&status,1)) {s->errors++;s->aht_valid=0;return;}
        if(!(status&8)) {
            /* Aosong AHT21 A1 manual: BE 08 00, wait at least 10ms unlocked. */
            uint8_t init[3]={0xBE,0x08,0};
            if(init_tries<3) {init_tries++;if(!I2cBus_Transfer(&app_i2c,APP_AHT_ADDRESS,init,3,0,0))next=now+10;}
            s->aht_valid=0;s->errors++;return;
        }
        init_tries=0;
        if(I2cBus_Transfer(&app_i2c,APP_AHT_ADDRESS,tx,3,0,0)) {s->errors++;s->aht_valid=0;return;}
        pending=1;deadline=now+100;return;
    }
    if((int32_t)(now-deadline)<0)return;pending=0;
    if(I2cBus_Transfer(&app_i2c,APP_AHT_ADDRESS,0,0,rx,7)||Aht21_Decode(rx,&s->temperature,&s->humidity)) {s->errors++;s->aht_valid=0;return;}
    s->aht_valid=1;s->updated_ms=now;
}
static void cs(int high) {HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,high?GPIO_PIN_SET:GPIO_PIN_RESET);}
int Board_FlashRead(uint32_t a,uint8_t *out,unsigned n) {
    uint8_t cmd[4];int rc;
    uint32_t cap=(app_flash_jedec&255U)>=21&&(app_flash_jedec&255U)<=24 ? (1U<<(app_flash_jedec&255U)):0;
    if((app_flash_jedec>>16)!=0xEF||!cap)return APP_UNAVAILABLE;
    if(a>=cap||n>cap-a||!out||n>65535)return APP_INVALID;
    cmd[0]=3;cmd[1]=(uint8_t)(a>>16);cmd[2]=(uint8_t)(a>>8);cmd[3]=(uint8_t)a;
    cs(0);rc=HAL_SPI_Transmit(&hspi1,cmd,4,20)==HAL_OK&&HAL_SPI_Receive(&hspi1,out,(uint16_t)n,20)==HAL_OK?APP_OK:APP_IO;cs(1);return rc;
}
static int flash_read(void *ctx,uint32_t a,uint8_t *out,unsigned n) {(void)ctx;return Board_FlashRead(a,out,n);}
#if APP_STORAGE_WRITE_ENABLED
static IsolatedFlash isolated;
static int raw_program(void *ctx,uint32_t a,const uint8_t *p,unsigned n) {
    uint8_t cmd[4],status=1;uint32_t start;int rc;(void)ctx;
    if(!n||n>256||(a&255)+n>256||a<APP_FLASH_REGION_START||a>=APP_FLASH_REGION_END||n>APP_FLASH_REGION_END-a)return APP_PROTECTED;
    cmd[0]=6;cs(0);rc=HAL_SPI_Transmit(&hspi1,cmd,1,20)==HAL_OK?APP_OK:APP_IO;cs(1);if(rc)return rc;
    cmd[0]=2;cmd[1]=(uint8_t)(a>>16);cmd[2]=(uint8_t)(a>>8);cmd[3]=(uint8_t)a;
    cs(0);rc=HAL_SPI_Transmit(&hspi1,cmd,4,20)==HAL_OK&&HAL_SPI_Transmit(&hspi1,(uint8_t *)p,(uint16_t)n,20)==HAL_OK?APP_OK:APP_IO;cs(1);if(rc)return rc;
    start=HAL_GetTick();
    while(status&1) {
        cmd[0]=5;cs(0);rc=HAL_SPI_Transmit(&hspi1,cmd,1,20)==HAL_OK&&HAL_SPI_Receive(&hspi1,&status,1,20)==HAL_OK?APP_OK:APP_IO;cs(1);if(rc)return rc;
        if((uint32_t)(HAL_GetTick()-start)>=100)return APP_TIMEOUT;
        if(status&1)vTaskDelay(pdMS_TO_TICKS(1));
    }return APP_OK;
}
#endif
void Board_StorageBackend(StorageBackend *io) {
    uint8_t cmd=0x9F,id[3]={0};
    cs(0);if(HAL_SPI_Transmit(&hspi1,&cmd,1,20)==HAL_OK&&HAL_SPI_Receive(&hspi1,id,3,20)==HAL_OK)app_flash_jedec=((uint32_t)id[0]<<16)|((uint32_t)id[1]<<8)|id[2];cs(1);
    memset(io,0,sizeof(*io));io->read=flash_read;io->start=0x2000;io->end=0x200000;
    /* No physical program/erase callbacks: history remains read-only. */
    io->writes_enabled=0;
#if APP_STORAGE_WRITE_ENABLED
    {
        IsolatedFlashConfig c;memset(&c,0,sizeof(c));c.read=flash_read;c.program=raw_program;c.start=APP_FLASH_REGION_START;c.end=APP_FLASH_REGION_END;c.approved=APP_FLASH_REGION_APPROVED;
        c.capacity=(app_flash_jedec&255U)>=21&&(app_flash_jedec&255U)<=24?(1U<<(app_flash_jedec&255U)):0;
        if((app_flash_jedec>>16)==0xEF)(void)IsolatedFlash_Init(&isolated,&c,io);
    }
#endif
}
#if APP_STORAGE_WRITE_ENABLED && !APP_FLASH_REGION_APPROVED
#error Set an explicitly approved isolated region before enabling physical writes
#endif

void Board_Magnetometer(SensorSnapshot *s) {
#if APP_MAGNETOMETER_HMC_CONFIRMED
    static uint8_t initialized;uint8_t reg=0x0A,id[3],rx[6],config[4]={0,0x70,0x20,0};int16_t x,y;
    if(!initialized) {
        if(I2cBus_Transfer(&app_i2c,0x1E,&reg,1,id,3)||memcmp(id,"H43",3)||I2cBus_Transfer(&app_i2c,0x1E,config,4,0,0)) {s->mag_valid=0;s->errors++;return;}
        initialized=1;
    }
    reg=3;if(I2cBus_Transfer(&app_i2c,0x1E,&reg,1,rx,6)) {s->mag_valid=0;s->errors++;return;}
    x=(int16_t)(((uint16_t)rx[0]<<8)|rx[1]);y=(int16_t)(((uint16_t)rx[4]<<8)|rx[5]);
    if(x==-4096||y==-4096||(!x&&!y)) {s->mag_valid=0;return;}
    {double heading=atan2((double)y,(double)x)*57.295779513;if(heading<0)heading+=360;s->heading=(int16_t)heading;s->mag_valid=1;}
#else
    s->mag_valid=0;
#endif
}
