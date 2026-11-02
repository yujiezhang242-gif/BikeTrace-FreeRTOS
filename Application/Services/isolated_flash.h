#ifndef ISOLATED_FLASH_H
#define ISOLATED_FLASH_H
#include "storage_service.h"
typedef struct {
    void *context;
    int (*read)(void *,uint32_t,uint8_t *,unsigned);
    int (*program)(void *,uint32_t,const uint8_t *,unsigned);
    uint32_t capacity,start,end;
    uint8_t approved;
} IsolatedFlashConfig;
typedef struct {
    IsolatedFlashConfig config;
    uint32_t journal,next_slot,sequence,last_address,last_ride;
    uint8_t last_stop,found;
} IsolatedFlash;
/* No erase operation is exposed. Existing history [0,0x200000) is immutable. */
int IsolatedFlash_Init(IsolatedFlash *f,const IsolatedFlashConfig *c,StorageBackend *backend);
#endif
