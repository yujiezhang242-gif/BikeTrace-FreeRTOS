#ifndef NET_FRAMER_H
#define NET_FRAMER_H
#include "app_types.h"
typedef struct {
    char data[NET_MAX_FRAME+1];
    unsigned length,depth;
    uint32_t errors,oversized;
    uint8_t quoted,escaped,dropping,complete;
} NetFramer;
void NetFramer_Init(NetFramer *f);
int NetFramer_Feed(NetFramer *f,uint8_t c);
#endif
