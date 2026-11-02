#include "isolated_flash.h"
#include <string.h>
static uint32_t u32(const uint8_t *p) {return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put(uint8_t *p,uint32_t n) {unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint32_t crc(const uint8_t *p,unsigned n) {uint32_t v=0xffffffff;unsigned i;while(n--) {v^=*p++;for(i=0;i<8;i++)v=(v>>1)^((v&1)?0xedb88320U:0);}return ~v;}
static int rd(void *ctx,uint32_t a,uint8_t *p,unsigned n) {IsolatedFlash *f=(IsolatedFlash *)ctx;if(a<f->config.start||a>f->config.end||n>f->config.end-a)return APP_INVALID;return f->config.read(f->config.context,a,p,n);}
static int program(IsolatedFlash *f,uint32_t a,const uint8_t *p,unsigned n) {
    uint8_t old[256];unsigned i;int rc;
    if(!f->config.approved)return APP_PROTECTED;
    if(!n||n>256||(a&255)+n>256||a<f->config.start||a>f->config.end||n>f->config.end-a)return APP_INVALID;
    rc=rd(f,a,old,n);if(rc)return rc;
    for(i=0;i<n;i++)if((old[i]&p[i])!=p[i])return APP_PROTECTED;
    rc=f->config.program(f->config.context,a,p,n);if(rc)return rc;
    rc=rd(f,a,old,n);if(rc)return rc;return memcmp(old,p,n)?APP_VERIFY:APP_OK;
}
static int prog(void *ctx,uint32_t a,const uint8_t *p,unsigned n) {
    IsolatedFlash *f=(IsolatedFlash *)ctx;
    if(a>f->journal||n>f->journal-a)return APP_PROTECTED;
    return program(f,a,p,n);
}
static int checkpoint(void *ctx,uint32_t ride,uint32_t address,int stopped) {
    IsolatedFlash *f=(IsolatedFlash *)ctx;uint8_t record[32];int rc;
    if(address<f->config.start||address>=f->journal||address%256)return APP_INVALID;
    if(f->next_slot>=128)return APP_FULL;
    memset(record,0xff,sizeof(record));put(record,0x42545243);put(record+4,f->sequence+1);put(record+8,ride);put(record+12,address);put(record+16,(uint32_t)(stopped!=0));put(record+20,crc(record,20));put(record+28,0xa5a5a5a5);
    rc=program(f,f->journal+f->next_slot*32,record,28);
    if(!rc)rc=program(f,f->journal+f->next_slot*32+28,record+28,4);
    /* Consume even interrupted slots; never overwrite a partial checkpoint. */
    f->next_slot++;
    if(rc)return rc;
    f->sequence++;f->last_address=address;f->last_ride=ride;f->last_stop=(uint8_t)(stopped!=0);f->found=1;return APP_OK;
}
int IsolatedFlash_Init(IsolatedFlash *f,const IsolatedFlashConfig *c,StorageBackend *io) {
    unsigned i,j;uint8_t record[32];int blank,rc;
    if(!f||!c||!io||!c->read||!c->program||!c->approved||c->start<0x200000U||c->start%4096||c->end%4096||c->end<=c->start||c->end-c->start<8192||c->end>c->capacity)return APP_PROTECTED;
    memset(f,0,sizeof(*f));f->config=*c;f->journal=c->end-4096;
    for(i=0;i<128;i++) {
        rc=rd(f,f->journal+i*32,record,32);if(rc)return rc;
        blank=1;for(j=0;j<32;j++)if(record[j]!=255)blank=0;if(blank)continue;
        f->next_slot=i+1;
        if(u32(record)!=0x42545243||u32(record+28)!=0xa5a5a5a5||u32(record+20)!=crc(record,20))continue;
        if(u32(record+12)<c->start||u32(record+12)>=f->journal||u32(record+12)%256)return APP_INVALID;
        if(!f->found||(int32_t)(u32(record+4)-f->sequence)>0) {f->found=1;f->sequence=u32(record+4);f->last_ride=u32(record+8);f->last_address=u32(record+12);f->last_stop=(uint8_t)u32(record+16);}
    }
    memset(io,0,sizeof(*io));io->context=f;io->read=rd;io->program=prog;io->checkpoint=checkpoint;
    io->start=f->found?f->last_address+256:c->start;io->end=f->journal;io->writes_enabled=1;return APP_OK;
}
