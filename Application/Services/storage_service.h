#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H
#include "storage_codec.h"
typedef struct {
    void *context;
    int (*read)(void *,uint32_t,uint8_t *,unsigned);
    int (*program)(void *,uint32_t,const uint8_t *,unsigned);
    /* Must persist and verify checkpoint; called after data readback only. */
    int (*checkpoint)(void *,uint32_t,uint32_t,int);
    uint32_t start,end;
    uint8_t writes_enabled;
} StorageBackend;
typedef struct {
    StorageBackend io;
    StorageSample samples[13];
    RideSummary summary;
    uint32_t address, ride_id, last_sample, last_stop, pages, failures, verifies;
    unsigned count;
    uint8_t have_sample, programmed, stopping, committed;
    uint8_t page[256], verify[256];
} StorageService;
void Storage_Init(StorageService *s,const StorageBackend *io);
int Storage_Handle(StorageService *s,const StorageRequest *r,StorageAck *ack);
/* Read only legacy recovery. Caller supplies validated index/table/count. */
int Storage_RestoreRead(const StorageBackend *io,uint32_t start,uint32_t end,uint16_t pages,RideSummary *summary);
#endif
