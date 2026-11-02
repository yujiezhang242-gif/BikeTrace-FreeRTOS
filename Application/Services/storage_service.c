#include "storage_service.h"
#include <string.h>
void Storage_Init(StorageService *s,const StorageBackend *io) {memset(s,0,sizeof(*s));s->io=*io;s->address=io->start;}
static int flush(StorageService *s,int stop) {
    int result;
    if(!s->io.writes_enabled)return APP_PROTECTED;
    if(!s->io.read||!s->io.program||!s->io.checkpoint)return APP_UNAVAILABLE;
    if(s->address%256||s->address<s->io.start||s->io.end<s->address||s->io.end-s->address<256)return APP_FULL;
    if(!s->programmed) {
        if(StorageCodec_EncodeLegacy(s->page,s->samples,s->count,&s->summary))return APP_INVALID;
        result=s->io.read(s->io.context,s->address,s->verify,256);if(result)return result;
        /* Never erase automatically. Already-identical pages are retry-safe. */
        if(memcmp(s->verify,s->page,256)) {
            unsigned i;for(i=0;i<256;i++)if((s->verify[i]&s->page[i])!=s->page[i])return APP_PROTECTED;
            result=s->io.program(s->io.context,s->address,s->page,256);if(result)return result;
        }
        result=s->io.read(s->io.context,s->address,s->verify,256);if(result)return result;
        if(memcmp(s->verify,s->page,256)) {s->verifies++;return APP_VERIFY;}
        s->programmed=1;
    }
    result=s->io.checkpoint(s->io.context,s->ride_id,s->address,stop);if(result)return result;
    s->programmed=0;s->address+=256;s->pages++;s->count=0;memset(s->samples,0,sizeof(s->samples));return APP_OK;
}
int Storage_Handle(StorageService *s,const StorageRequest *r,StorageAck *a) {
    int rc=APP_OK;
    a->request_id=r->request_id;a->address=s->address;a->result=APP_INVALID;
    if(r->op!=STORE_SAMPLE&&r->op!=STORE_STOP)return APP_INVALID;
    if(r->op==STORE_SAMPLE&&!r->sample_valid)return APP_INVALID;
    if(s->committed&&r->ride_id==s->ride_id) {
        if(r->op==STORE_STOP&&r->request_id==s->last_stop) {a->result=APP_OK;return APP_OK;}
        return APP_INVALID;
    }
    if(s->ride_id!=r->ride_id) {
        if(s->count||s->programmed||s->stopping)return APP_FULL;
        s->ride_id=r->ride_id;s->have_sample=0;s->committed=0;
    }
    /* STOP retry preserves the frozen buffer and does not append its sample twice. */
    if(s->stopping) {
        if(r->op!=STORE_STOP) {a->result=APP_FULL;return APP_FULL;}
        rc=flush(s,1);
        if(!rc) {s->stopping=0;s->committed=1;s->last_stop=r->request_id;}
        goto done;
    }
    /* Failed full page must commit before accepting anything else. */
    if(s->count==13||s->programmed) {rc=flush(s,s->stopping);if(rc)goto done;}
    if(r->sample_valid&&(!s->have_sample||s->last_sample!=r->sample.sample_id)) {
        s->samples[s->count++]=r->sample;s->last_sample=r->sample.sample_id;s->have_sample=1;
    }
    s->summary=r->summary;
    if(r->op==STORE_STOP) {s->stopping=1;s->last_stop=r->request_id;}
    if(s->count==13||s->stopping)rc=flush(s,s->stopping);
    if(rc==APP_OK&&s->stopping) {s->stopping=0;s->committed=1;}
 done:
    if(rc)s->failures++;a->result=rc;return rc;
}
int Storage_RestoreRead(const StorageBackend *io,uint32_t start,uint32_t end,uint16_t pages,RideSummary *out) {
    uint8_t page[256];StorageSample samples[13];unsigned n;uint32_t address;
    if(!io||!io->read||!out||!pages||pages==0xffff||start%256||end<start||(uint32_t)pages>(end-start)/256)return APP_INVALID;
    address=start+((uint32_t)pages-1)*256;
    if(io->read(io->context,address,page,256))return APP_IO;
    return StorageCodec_DecodeLegacy(page,samples,&n,out);
}
