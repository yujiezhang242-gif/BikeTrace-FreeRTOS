#include "app_runtime.h"
#include "usart.h"
#include "task.h"
#include <string.h>
volatile uint32_t app_gps_last_rx_ms;
static uint8_t rx[3];
static char net_tx[512],debug_tx[160];
static volatile uint8_t net_busy,debug_busy;
static uint32_t net_started,debug_started;
StreamBufferHandle_t app_gps_stream,app_net_stream,app_debug_stream;
void App_UartStart(unsigned n) {UART_HandleTypeDef *u=n==1?&huart1:n==2?&huart2:&huart3;configASSERT(n>=1&&n<=3);configASSERT(HAL_UART_Receive_IT(u,&rx[n-1],1)==HAL_OK);}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *u) {
    BaseType_t wake=pdFALSE;StreamBufferHandle_t stream=0;unsigned i;
    if(u->Instance==USART1) {i=0;stream=app_debug_stream;}else if(u->Instance==USART2) {i=1;stream=app_net_stream;}else if(u->Instance==USART3) {i=2;stream=app_gps_stream;app_gps_last_rx_ms=HAL_GetTick();}else return;
    if(stream&&xStreamBufferSendFromISR(stream,&rx[i],1,&wake)!=1) {
        if(i==0)app_health.debug_rx_overflow++;else if(i==1)app_health.network_rx_overflow++;else app_health.gps_rx_overflow++;
    }
    if(HAL_UART_Receive_IT(u,&rx[i],1)!=HAL_OK)app_health.uart_errors++;
    portYIELD_FROM_ISR(wake);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *u) {
    unsigned i;app_health.uart_errors++;
    if(u->Instance==USART1)i=0;else if(u->Instance==USART2)i=1;else if(u->Instance==USART3)i=2;else return;
    /* HAL error cleanup may already restart reception; HAL_BUSY is harmless here. */
    (void)HAL_UART_Receive_IT(u,&rx[i],1);
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *u) {if(u->Instance==USART2)net_busy=0;if(u->Instance==USART1)debug_busy=0;}
int App_NetSend(void *ctx,const char *s,unsigned n) {
    (void)ctx;
    if(net_busy) {if((uint32_t)(HAL_GetTick()-net_started)>2000) {HAL_UART_AbortTransmit(&huart2);net_busy=0;}else return APP_FULL;}
    if(!s||n>sizeof(net_tx))return APP_INVALID;memcpy(net_tx,s,n);net_busy=1;net_started=HAL_GetTick();
    if(HAL_UART_Transmit_IT(&huart2,(uint8_t *)net_tx,(uint16_t)n)!=HAL_OK) {net_busy=0;return APP_IO;}return APP_OK;
}
int App_DebugSend(const char *s,unsigned n) {
    if(debug_busy) {if((uint32_t)(HAL_GetTick()-debug_started)>2000) {HAL_UART_AbortTransmit(&huart1);debug_busy=0;}else return APP_FULL;}
    if(!s||n>sizeof(debug_tx))return APP_INVALID;memcpy(debug_tx,s,n);debug_busy=1;debug_started=HAL_GetTick();
    if(HAL_UART_Transmit_IT(&huart1,(uint8_t *)debug_tx,(uint16_t)n)!=HAL_OK) {debug_busy=0;return APP_IO;}return APP_OK;
}
