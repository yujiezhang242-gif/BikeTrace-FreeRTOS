#include "legacy_reader.h"
#include <string.h>
static uint16_t u16(const uint8_t *p) {return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t u32(const uint8_t *p) {return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int Legacy_ReadIndex(const StorageBackend *io,LegacyIndex *out) {
    uint8_t p[76];unsigned sector,page,i;uint32_t version;int corrupt=0;
    memset(out,0,sizeof(*out));if(!io||!io->read)return APP_INVALID;
    for(sector=0;sector<2;sector++)for(page=0;page<16;page++) {
        if(io->read(io->context,sector*4096+page*256,p,76))return APP_IO;
        if(p[75]!=0xA5)continue;
        if(u16(p)>16||u16(p+2)>=16||u16(p+72)>=16||p[74]>1) {corrupt=1;continue;}
        version=u32(p+68);
        if(!out->valid||(int32_t)(version-out->version)>0) {
            out->count=u16(p);out->next_table=u16(p+2);out->page=u16(p+72);out->sector=p[74];out->version=version;out->valid=1;
            for(i=0;i<16;i++)out->created[i]=u32(p+4+i*4);
        }
    }return out->valid?APP_OK:corrupt?APP_INVALID:APP_UNAVAILABLE;
}
int Legacy_TableBounds(const LegacyIndex *index,unsigned n,uint32_t *start,uint32_t *end) {
    static const uint32_t a[17]={0x2000,0x21000,0x40000,0x60000,0x80000,0xa0000,0xc0000,0xe0000,0x100000,0x120000,0x140000,0x160000,0x180000,0x1a0000,0x1c0000,0x1e0000,0x200000};unsigned slot;
    if(!index||!index->valid||!n||n>index->count)return APP_INVALID;
    /* Stored index points at its current table; old init advances next_table once. */
    slot=index->count==16?(index->next_table+n)%16:n-1;*start=a[slot];*end=a[slot+1];return APP_OK;
}
