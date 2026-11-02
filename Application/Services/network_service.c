#include "network_service.h"
#include <string.h>
#include <stdio.h>
static int due(uint32_t now,uint32_t deadline) {return (int32_t)(now-deadline)>=0;}
static void backoff(NetworkService *n,uint32_t now) {
    uint32_t shift=n->retries<6?n->retries:6;n->retries++;n->state=NET_BACKOFF;n->deadline=now+(1000U<<shift);n->waiting=0;n->line_len=0;
    NetFramer_Init(&n->framer);
}
void Network_RxOverflow(NetworkService *n,uint32_t now) {n->malformed++;if(n->state!=NET_OFF)backoff(n,now);}
void Network_Init(NetworkService *n,const NetTransport *io) {memset(n,0,sizeof(*n));n->io=*io;n->state=io->enabled?NET_INIT:NET_OFF;}
uint32_t Network_Utc(const NetworkService *n,uint32_t now) {return n->has_time?n->utc+(uint32_t)(now-n->utc_received_ms)/1000:0;}
void Network_Tick(NetworkService *n,uint32_t now) {
    static const char *const commands[]={"AT\r","ATE0\r","AT+CPIN?\r","AT+CREG?\r","AT+QICSGP=1,1\r","AT+QIDEACT=1\r","AT+QIACT=1\r"};
    char open[100];const char *cmd;
    if(n->state==NET_OFF)return;
    if(n->framer.depth&&(uint32_t)(now-n->last_rx)>1500) {n->malformed++;NetFramer_Init(&n->framer);}
    if(n->state==NET_CONNECTED)return;
    if(n->state==NET_BACKOFF) {if(!due(now,n->deadline))return;n->command=0;n->state=NET_INIT;}
    if(n->waiting) {if(due(now,n->deadline)) {n->timeouts++;backoff(n,now);}return;}
    if(!n->io.host||!n->io.host[0]||n->io.port==0||n->io.port>65535) {n->state=NET_OFF;return;}
    if(n->command<7)cmd=commands[n->command];
    else {int k=snprintf(open,sizeof(open),"AT+QIOPEN=1,0,\"TCP\",\"%s\",%u,0,2\r",n->io.host,n->io.port);if(k<0||(unsigned)k>=sizeof(open)) {n->state=NET_OFF;return;}cmd=open;}
    if(!n->io.send||n->io.send(n->io.context,cmd,(unsigned)strlen(cmd))!=APP_OK) {backoff(n,now);return;}
    n->detail=0;n->waiting=1;n->deadline=now+(n->command>=4?30000:3000);
    n->state=n->command<2?NET_INIT:n->command<4?NET_REGISTERING:NET_CONNECTING;
}
int Network_Byte(NetworkService *n,uint8_t c,uint32_t now) {
    int result=0;n->last_rx=now;
    if(n->state==NET_CONNECTED) {
        int frame=NetFramer_Feed(&n->framer,c);
        if(frame==1) {
            uint32_t utc=0;result=NetProtocol_Parse(&n->json,n->framer.data,n->framer.length,&utc,&n->nav);
            if(result<0) {n->malformed++;result=0;}
            if(result==1) {n->utc=utc;n->utc_received_ms=now;n->has_time=1;}
        }
    }
    if(c!='\n') {
        if(n->line_len<sizeof(n->line)-1)n->line[n->line_len++]=(char)c;
        else n->line_len=0;
        return result;
    }
    while(n->line_len&&n->line[n->line_len-1]=='\r')n->line_len--;
    n->line[n->line_len]=0;
    if(!strcmp(n->line,"CLOSED")||!strcmp(n->line,"NO CARRIER")||!strcmp(n->line,"+QIURC: \"closed\",0"))backoff(n,now);
    else if(n->waiting) {
        if(!strcmp(n->line,"ERROR")||!strncmp(n->line,"+CME ERROR:",11)) {
            if(n->command==5) {n->command++;n->waiting=0;}else backoff(n,now);
        }
        else if(n->command==2&&!strcmp(n->line,"+CPIN: READY"))n->detail=1;
        else if(n->command==3&&(!strcmp(n->line,"+CREG: 0,1")||!strcmp(n->line,"+CREG: 0,5")))n->detail=1;
        else if(n->command==7&&!strcmp(n->line,"CONNECT")) {n->state=NET_CONNECTED;n->waiting=0;}
        else if(n->command<7&&!strcmp(n->line,"OK")) {
            if((n->command==2||n->command==3)&&!n->detail)backoff(n,now);
            else {n->command++;n->waiting=0;}
        }
    }
    n->line_len=0;return result;
}
int Network_Send(NetworkService *n,const char *s,unsigned len,uint32_t deadline,uint32_t now) {
    if(due(now,deadline))return APP_TIMEOUT;
    if(n->state!=NET_CONNECTED)return APP_UNAVAILABLE;
    if(len>NET_MAX_FRAME)return APP_INVALID;
    return n->io.send?n->io.send(n->io.context,s,len):APP_UNAVAILABLE;
}
