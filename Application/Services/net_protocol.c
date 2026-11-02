#include "net_protocol.h"
#include "geo.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static void ws(JsonWorkspace *w) {while(w->pos<w->length&&(w->text[w->pos]==' '||w->text[w->pos]=='\r'||w->text[w->pos]=='\n'||w->text[w->pos]=='\t'))w->pos++;}
static int parse(JsonWorkspace *w,unsigned depth) {
    unsigned id,start;char c,close;JsonToken *t;
    if(depth>12||w->used>=144)return -1;
    ws(w);if(w->pos>=w->length)return -1;
    id=w->used++;t=&w->tokens[id];memset(t,0,sizeof(*t));t->start=(uint16_t)w->pos;c=w->text[w->pos++];t->type=(uint8_t)c;
    if(c=='{'||c=='[') {
        close=c=='{'?'}':']';ws(w);
        if(w->pos<w->length&&w->text[w->pos]==close)w->pos++;
        else for(;;) {
            if(c=='{') {
                ws(w);if(w->pos>=w->length||w->text[w->pos]!='"'||parse(w,depth+1)<0)return -1;
                ws(w);if(w->pos>=w->length||w->text[w->pos++]!=':')return -1;
            }
            if(parse(w,depth+1)<0||t->count==255)return -1;t->count++;
            ws(w);if(w->pos>=w->length)return -1;
            if(w->text[w->pos++]==close)break;
            if(w->text[w->pos-1]!=',')return -1;
        }
    } else if(c=='"') {
        int closed=0;
        while(w->pos<w->length) {
            unsigned char a=(unsigned char)w->text[w->pos++];
            if(a=='"') {closed=1;break;}if(a<32)return -1;
            if(a=='\\') {
                unsigned i;if(w->pos>=w->length)return -1;a=(unsigned char)w->text[w->pos++];
                if(a=='u') {for(i=0;i<4;i++) {if(w->pos>=w->length)return -1;a=(unsigned char)w->text[w->pos++];if(!((a>='0'&&a<='9')||(a>='a'&&a<='f')||(a>='A'&&a<='F')))return -1;}}
                else if(!a||!strchr("\"\\/bfnrt",a))return -1;
            }
        } if(!closed)return -1;
    } else {
        start=w->pos-1;
        if(c=='t'||c=='f'||c=='n') {
            const char *literal=c=='t'?"true":c=='f'?"false":"null";unsigned n=(unsigned)strlen(literal);
            if(start+n>w->length||memcmp(w->text+start,literal,n))return -1;w->pos=start+n;
        } else {
            w->pos=start;if(w->text[w->pos]=='-')w->pos++;
            if(w->pos>=w->length)return -1;
            if(w->text[w->pos]=='0')w->pos++;
            else {if(w->text[w->pos]<'1'||w->text[w->pos]>'9')return -1;while(w->pos<w->length&&w->text[w->pos]>='0'&&w->text[w->pos]<='9')w->pos++;}
            if(w->pos<w->length&&w->text[w->pos]=='.') {unsigned p=++w->pos;while(w->pos<w->length&&w->text[w->pos]>='0'&&w->text[w->pos]<='9')w->pos++;if(p==w->pos)return -1;}
            if(w->pos<w->length&&(w->text[w->pos]=='e'||w->text[w->pos]=='E')) {unsigned p;w->pos++;if(w->pos<w->length&&(w->text[w->pos]=='+'||w->text[w->pos]=='-'))w->pos++;p=w->pos;while(w->pos<w->length&&w->text[w->pos]>='0'&&w->text[w->pos]<='9')w->pos++;if(p==w->pos)return -1;}
            t->type='0';
        }
    }
    t->end=(uint16_t)w->pos;t->next=(uint16_t)w->used;return (int)id;
}
static int eq(JsonWorkspace *w,int id,const char *s) {
    JsonToken *t;if(id<0)return 0;t=&w->tokens[id];
    return t->type=='"'&&t->end-t->start==(unsigned)strlen(s)+2&&!memcmp(w->text+t->start+1,s,strlen(s));
}
static int field(JsonWorkspace *w,int object,const char *key) {
    unsigned i,n;int found=-1;if(object<0||w->tokens[object].type!='{')return -1;
    i=(unsigned)object+1;
    for(n=0;n<w->tokens[object].count;n++) {unsigned v=i+1;if(eq(w,(int)i,key)) {if(found>=0)return -1;found=(int)v;}i=w->tokens[v].next;}
    return found;
}
static int number(JsonWorkspace *w,int id,double min,double max,double *v) {
    char s[40];unsigned n;if(id<0||w->tokens[id].type!='0')return 0;n=w->tokens[id].end-w->tokens[id].start;
    if(n>=sizeof(s))return 0;memcpy(s,w->text+w->tokens[id].start,n);s[n]=0;*v=strtod(s,0);return *v>=min&&*v<=max;
}
static int u32(JsonWorkspace *w,int id,uint32_t *v) {double n;if(!number(w,id,0,4294967295.0,&n))return 0;*v=(uint32_t)n;return n==(double)*v;}
static int small(JsonWorkspace *w,int id,char out[4]) {
    unsigned n,i;if(id<0||w->tokens[id].type!='"')return 0;n=w->tokens[id].end-w->tokens[id].start-2;
    if(!n||n>3)return 0;for(i=0;i<n;i++) {char c=w->text[w->tokens[id].start+1+i];if(c<'A'||c>'Z')return 0;out[i]=c;}out[n]=0;return 1;
}
int NetProtocol_Parse(JsonWorkspace *w,const char *s,unsigned length,uint32_t *utc,Navigation *nav) {
    int cmd,data,paths,steps;unsigned i,id;Navigation candidate;
    if(!s||!length||length>NET_MAX_FRAME)return APP_INVALID;
    w->used=0;w->pos=0;w->text=s;w->length=length;
    if(parse(w,0)!=0)return APP_INVALID;ws(w);if(w->pos!=length||w->tokens[0].type!='{')return APP_INVALID;
    cmd=field(w,0,"cmd");if(eq(w,cmd,"time response"))return u32(w,field(w,0,"time"),utc)?1:APP_INVALID;
    if(!eq(w,cmd,"path_plan"))return APP_INVALID;
    data=field(w,0,"data");paths=field(w,data,"paths");steps=field(w,paths,"steps");
    if(steps<0||w->tokens[steps].type!='['||!w->tokens[steps].count||w->tokens[steps].count>NAV_MAX_STEPS)return APP_INVALID;
    memset(&candidate,0,sizeof(candidate));candidate.count=w->tokens[steps].count;
    if(!u32(w,field(w,paths,"distance"),&candidate.distance)||candidate.distance>1000000||!u32(w,field(w,paths,"duration"),&candidate.seconds))return APP_INVALID;
    /* Legacy source places destination in data.paths. */
    if(!number(w,field(w,paths,"dest_lat"),-90,90,&candidate.latitude)||!number(w,field(w,paths,"dest_lng"),-180,180,&candidate.longitude))return APP_INVALID;
    id=(unsigned)steps+1;
    for(i=0;i<candidate.count;i++) {
        if(!small(w,field(w,(int)id,"inst"),candidate.steps[i].instruction)||!small(w,field(w,(int)id,"orie"),candidate.steps[i].orientation)||!u32(w,field(w,(int)id,"dist"),&candidate.steps[i].distance)||candidate.steps[i].distance>1000000)return APP_INVALID;
        id=w->tokens[id].next;
    }
    candidate.version=nav->version+1;*nav=candidate;return 2;
}
int NetProtocol_Upload(char *out,unsigned cap,const BikeSnapshot *s,const GpsFix *f,uint32_t utc) {
    int n;double latitude,longitude;Geo_Gcj02(f->latitude_deg,f->longitude_deg,&latitude,&longitude);
    n=snprintf(out,cap,"{\"cmd\":\"upload data\",\"name\":\"dev_bike\",\"type\":\"bike\",\"time\":%lu,\"data\":{\"temp\":%.2f,\"humidity\":%.2f,\"speed\":%.2f,\"distance\":%.2f,\"total_time\":%lu,\"state\":%u,\"max_speed\":%.2f,\"average_speed\":%.2f,\"kcal\":%.2f,\"lat\":%.7f,\"lng\":%.7f}}",
        (unsigned long)utc,s->temperature/100.0,s->humidity/100.0,s->speed_millikmh/1000.0,s->distance_mm/1000000.0,(unsigned long)(s->elapsed_ms/1000),s->state==BIKE_IDLE?0:s->state==BIKE_RIDE?1:2,s->max_speed_millikmh/1000.0,s->average_speed_millikmh/1000.0,s->distance_mm/1000000.0*35.0,latitude,longitude);
    return n<0||(unsigned)n>=cap?APP_FULL:n;
}
