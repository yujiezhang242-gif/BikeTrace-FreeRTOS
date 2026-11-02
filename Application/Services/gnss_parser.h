#ifndef GNSS_PARSER_H
#define GNSS_PARSER_H
#include "app_types.h"
typedef struct {
    char frame[128];
    uint16_t length;
    uint8_t collecting, dropping;
    uint32_t good, bad, oversized;
} GnssParser;
void GnssParser_Reset(GnssParser *p);
/* 1: checksum-valid RMC including V; 0: incomplete; -1: rejected. */
int GnssParser_Feed(GnssParser *p, uint8_t byte, uint32_t now, GpsFix *out);
#endif
