#ifndef STORAGE_CODEC_H
#define STORAGE_CODEC_H
#include "app_types.h"
int StorageCodec_EncodeLegacy(uint8_t page[256],const StorageSample *samples,unsigned count,const RideSummary *summary);
int StorageCodec_DecodeLegacy(const uint8_t page[256],StorageSample *samples,unsigned *count,RideSummary *summary);
#endif
