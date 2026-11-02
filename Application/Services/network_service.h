#ifndef NETWORK_SERVICE_H
#define NETWORK_SERVICE_H
#include "net_framer.h"
#include "net_protocol.h"
typedef enum {NET_OFF,NET_INIT,NET_REGISTERING,NET_CONNECTING,NET_CONNECTED,NET_BACKOFF} NetState;
typedef struct {
    void *context;
    int (*send)(void *,const char *,unsigned);
    const char *host;
    unsigned port;
    uint8_t enabled;
} NetTransport;
typedef struct {
    NetTransport io;
    NetState state;
    NetFramer framer;
    JsonWorkspace json;
    Navigation nav;
    char line[128];
    unsigned line_len,command;
    uint32_t deadline,last_rx,retries,timeouts,malformed,utc,utc_received_ms;
    uint8_t waiting,detail,has_time;
} NetworkService;
void Network_Init(NetworkService *n,const NetTransport *io);
void Network_RxOverflow(NetworkService *n,uint32_t now);
void Network_Tick(NetworkService *n,uint32_t now);
/* 1 updated time, 2 updated nav, 0 other. */
int Network_Byte(NetworkService *n,uint8_t c,uint32_t now);
uint32_t Network_Utc(const NetworkService *n,uint32_t now);
int Network_Send(NetworkService *n,const char *message,unsigned length,uint32_t deadline,uint32_t now);
#endif
