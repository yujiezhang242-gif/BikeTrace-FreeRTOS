#ifndef LEGACY_READER_H
#define LEGACY_READER_H
#include "storage_service.h"
typedef struct {
    uint16_t count,next_table,page;uint8_t sector;
    uint32_t version,created[16];uint8_t valid;
} LegacyIndex;
int Legacy_ReadIndex(const StorageBackend *io,LegacyIndex *out);
int Legacy_TableBounds(const LegacyIndex *index,unsigned table,uint32_t *start,uint32_t *end);
#endif
