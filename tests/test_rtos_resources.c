/* Real FreeRTOS queue/stream/heap code with a single-thread test port.
   Tests allocation and FIFO semantics only; no scheduler/interrupt simulation. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"
#include "app_types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
uint32_t SystemCoreClock=72000000;static unsigned malloc_failures;
void App_Assert(const char *f,int n) {fprintf(stderr,"RTOS assert %s:%d\n",f,n);abort();}
void vApplicationMallocFailedHook(void) {malloc_failures++;}
void vApplicationStackOverflowHook(TaskHandle_t t,char *n) {(void)t;(void)n;abort();}
StackType_t *pxPortInitialiseStack(StackType_t *s,TaskFunction_t fn,void *p) {(void)fn;(void)p;return s;}
BaseType_t xPortStartScheduler(void) {return pdFALSE;}
void vPortEndScheduler(void) {}
int main(void) {
    QueueHandle_t q,latest;StreamBufferHandle_t stream;StorageRequest r,out;BaseType_t wake=pdFALSE;uint8_t bytes[20],read[20];void *blocks[100];unsigned i,n=0;size_t before;
    {void *init=pvPortMalloc(1);assert(init);vPortFree(init);}before=xPortGetFreeHeapSize();
    q=xQueueCreate(5,sizeof(r));latest=xQueueCreate(1,sizeof(uint32_t));stream=xStreamBufferCreate(8,1);assert(q&&latest&&stream);
    memset(&r,0,sizeof(r));
    /* Same reserved-slot policy as ControlTask; STOP follows every accepted sample. */
    for(i=0;i<4;i++) {assert(uxQueueSpacesAvailable(q)>1);r.sample.sample_id=i;r.op=STORE_SAMPLE;assert(xQueueSend(q,&r,0)==pdTRUE);}
    assert(uxQueueSpacesAvailable(q)==1);r.op=STORE_STOP;r.request_id=77;assert(xQueueSend(q,&r,0)==pdTRUE);assert(xQueueSend(q,&r,0)==errQUEUE_FULL);
    for(i=0;i<4;i++){assert(xQueueReceive(q,&out,0)==pdTRUE);assert(out.op==STORE_SAMPLE&&out.sample.sample_id==i);}assert(xQueueReceive(q,&out,0)==pdTRUE&&out.op==STORE_STOP&&out.request_id==77);
    i=1;assert(xQueueOverwrite(latest,&i)==pdTRUE);i=2;assert(xQueueOverwrite(latest,&i)==pdTRUE);i=0;assert(xQueueReceive(latest,&i,0)==pdTRUE&&i==2);
    memset(bytes,3,sizeof(bytes));assert(xStreamBufferSendFromISR(stream,bytes,20,&wake)==8);assert(xStreamBufferSendFromISR(stream,bytes,1,&wake)==0);assert(xStreamBufferReceive(stream,read,20,0)==8);
    while(n<100&&(blocks[n]=pvPortMalloc(256))!=0)n++;
    assert(n>0&&malloc_failures==1&&xPortGetMinimumEverFreeHeapSize()<before);
    for(i=0;i<n;i++)vPortFree(blocks[i]);vQueueDelete(q);vQueueDelete(latest);vStreamBufferDelete(stream);assert(xPortGetFreeHeapSize()==before);
    puts("T12: actual FreeRTOS FIFO STOP reservation, latest snapshot overwrite, FromISR stream overflow, heap exhaustion/recovery PASS (test port; scheduling NOT_RUN)");return 0;
}
