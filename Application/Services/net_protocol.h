#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H
#include "app_types.h"
typedef struct {char instruction[4],orientation[4];uint32_t distance;} NavStep;
typedef struct {
    NavStep steps[NAV_MAX_STEPS];
    unsigned count;
    uint32_t distance,seconds,version;
    double latitude,longitude;
} Navigation;
/* Fixed parser workspace, one Network owner, no heap/cJSON dependency. */
typedef struct {uint16_t start,end,next;uint8_t type,count;} JsonToken;
typedef struct {JsonToken tokens[144];unsigned used,pos;const char *text;unsigned length;} JsonWorkspace;
/* Returns 1 time, 2 navigation, negative rejection. Existing nav is preserved on failure. */
int NetProtocol_Parse(JsonWorkspace *w,const char *json,unsigned length,uint32_t *utc,Navigation *nav);
int NetProtocol_Upload(char *out,unsigned capacity,const BikeSnapshot *s,const GpsFix *fix,uint32_t utc);
#endif
