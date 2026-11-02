#include "net_framer.h"
#include <string.h>
void NetFramer_Init(NetFramer *f) {memset(f,0,sizeof(*f));}
int NetFramer_Feed(NetFramer *f,uint8_t c) {
    if(f->complete) { f->length=0;f->depth=0;f->quoted=0;f->escaped=0;f->dropping=0;f->complete=0; }
    if(!f->depth) {if(c!='{')return 0;f->depth=1;f->length=0;f->data[f->length++]='{';return 0;}
    if(!f->dropping) {
        if(f->length>=NET_MAX_FRAME) {f->oversized++;f->dropping=1;}
        else f->data[f->length++]=(char)c;
    }
    if(f->quoted) {
        if(f->escaped)f->escaped=0;else if(c=='\\')f->escaped=1;else if(c=='"')f->quoted=0;
    } else {
        if(c=='"')f->quoted=1;
        else if(c=='{'||c=='[') {f->depth++;if(f->depth>16&&!f->dropping) {f->errors++;f->dropping=1;}}
        else if(c=='}'||c==']')f->depth--;
    }
    if(!f->depth) {
        f->complete=1;
        if(f->dropping) {f->errors++;return -1;}
        f->data[f->length]=0;return 1;
    }
    return 0;
}
