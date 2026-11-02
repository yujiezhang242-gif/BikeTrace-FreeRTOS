#ifndef TEST_PORTMACRO_H
#define TEST_PORTMACRO_H
#include <stdint.h>
#include <stddef.h>
typedef uintptr_t StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
#define portPOINTER_SIZE_TYPE uintptr_t
#define portMAX_DELAY UINT32_MAX
#define portSTACK_GROWTH (-1)
#define portBYTE_ALIGNMENT 8
#define portTICK_PERIOD_MS (1000U/configTICK_RATE_HZ)
#define portCRITICAL_NESTING_IN_TCB 0
#define portENTER_CRITICAL() ((void)0)
#define portEXIT_CRITICAL() ((void)0)
#define portDISABLE_INTERRUPTS() ((void)0)
#define portENABLE_INTERRUPTS() ((void)0)
#define portSET_INTERRUPT_MASK_FROM_ISR() 0U
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x) ((void)(x))
#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID() ((void)0)
#define portYIELD() ((void)0)
#define portYIELD_WITHIN_API() ((void)0)
#define portYIELD_FROM_ISR(x) ((void)(x))
#define portNOP() ((void)0)
#define portINLINE inline
#define portTASK_FUNCTION_PROTO(fn,arg) void fn(void *arg)
#define portTASK_FUNCTION(fn,arg) void fn(void *arg)
#endif
