#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H
#include "app_types.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
typedef struct {
    uint32_t heartbeat_500,heartbeat_1000,control_cycles,service_cycles,ui_cycles;
    uint32_t gps_rx_overflow,network_rx_overflow,debug_rx_overflow,uart_errors;
    uint32_t gps_good,gps_bad,gps_oversized,storage_queue_full,storage_failures,flash_verify_failures;
    uint32_t net_retries,net_timeouts,net_bad,ui_errors,sensor_errors;
    uint32_t heap_free,heap_min,control_stack,service_stack,ui_stack;
    uint32_t assert_line, fault,control_latency_max_ms;
    const char *assert_file;
} AppHealth;
extern volatile AppHealth app_health;
extern volatile uint32_t app_gps_last_rx_ms;
extern StreamBufferHandle_t app_gps_stream,app_net_stream,app_debug_stream;
void App_Start(void);
void App_UartStart(unsigned channel);
int App_NetSend(void *ctx,const char *data,unsigned length);
int App_DebugSend(const char *data,unsigned length);
#endif
