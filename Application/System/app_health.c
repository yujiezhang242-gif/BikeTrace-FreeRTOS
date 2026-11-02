#include "app_runtime.h"
#include "task.h"
#include "main.h"
volatile AppHealth app_health;
void App_Assert(const char *file,int line) {
    taskDISABLE_INTERRUPTS();app_health.assert_file=file;app_health.assert_line=(uint32_t)line;if(!app_health.fault)app_health.fault=1;
    for(;;) {__NOP();}
}
void vApplicationMallocFailedHook(void) {app_health.fault=2;App_Assert("malloc",0);}
void vApplicationStackOverflowHook(TaskHandle_t task,char *name) {(void)task;(void)name;app_health.fault=3;App_Assert("stack",0);}
