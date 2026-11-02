# 异常与IRQ唯一责任表

向量引用来自 MDK-ARM/startup_stm32f103xb.s；启动文件的弱默认入口保留。

|入口|最终强实现来源|优先级/约束|
|---|---|---|
|SVC_Handler|FreeRTOS portable/RVDS/ARM_CM3/port.c，通过FreeRTOSConfig.h别名vPortSVCHandler|内核|
|PendSV_Handler|同一port，别名xPortPendSVHandler|15，内核|
|SysTick_Handler|同一port，别名xPortSysTickHandler|15，1kHz RTOS tick；不调用HAL_IncTick|
|TIM4_IRQHandler|Application/System/hal_timebase_tim4.c|4，唯一HAL_IncTick入口；不调用RTOS API|
|USART1/2/3_IRQHandler|Core/Src/stm32f1xx_it.c → HAL_UART_IRQHandler → app_uart.c回调|6，可调用FromISR，溢出计数，重启IT接收|
|TIM2_IRQHandler|原Core处理函数保留，但不初始化/启动TIM2|不作为业务时间源|
|DMA1_Channel3/6_IRQHandler|原HAL处理函数保留；不启动DMA接收|尚未迁移DMA|
|HardFault等|原Core/Src/stm32f1xx_it.c|异常停机；需硬件观察|

NVIC分组沿用HAL NVIC_PRIORITYGROUP_4；4个优先级位。max syscall逻辑5（编码0x50），允许FromISR范围5–15；UART6满足，TIM4=4不能调用。从ISR唤醒由portYIELD_FROM_ISR统一处理。

project_audit.py检查旧Core中没有第二个SysTick/PendSV/SVC定义；工程仅列一个port和heap_4。Clang对象检查使用官方GCC port验证C集成，不能证明ARMCC5的RVDS汇编链接已通过。

HAL_InitTick强覆盖HAL弱默认实现，在HAL_Init和时钟变更时均重配TIM4，运行前后有独立HAL超时。低功耗/动态时钟切换尚未验收。
