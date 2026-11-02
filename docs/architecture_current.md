# 当前裸机调用链

main → HAL_Init → SystemClock_Config → GPIO/TIM2/UART1/2/3/SPI1（DMA init 注释）→ debug_uart_init/u2.start/u3.start → iic.init → aht21.init → tim.start → bike_init → hmc5883l.init/calibration。

bike_init → screen.init / at24c02.init / ec600n_tcp.init / init_flash / bike_reset。
init_flash → W25Qx init/ID → init_index → read_index_from_flash → 条件 set_next_index（可能擦索引）→ 条件 clear_ride_record_count（写 EEPROM）。

main 循环：GPS完整标志 → parse_gprmc_data；bike_handle → cloud_recv / list / export；TIM2标志 → test("123",模拟里程) → clearTimFlag。

bike_handle_key、bike_update 在 bike_handle 中均被注释。未启用的 bike_update 内包含网络连接/时间请求、累计秒数、GPS速度距离、温湿度、上传、save_item、屏幕刷新；自动 restore_last_ride_status 调用也注释。保留 IDLE/RIDE/STOP 代码不代表当前固件可正常启停骑行。

UART IRQ → HAL_UART_IRQHandler → HAL_UART_RxCpltCallback：U1直接命令解释和擦写，U2写 AT ringbuffer，U3组GPS帧，随后重新启动单字节 Receive_IT。SysTick 在 stm32f1xx_it.c 调用 HAL_IncTick；SVC/PendSV 在同文件为空实现；启动汇编提供向量引用及弱默认入口。TIM2经HAL回调设置单比特业务标志。

状态所有权尚未完成任务隔离：bike.c 静态 bike 经公共 setter 被 network/restore 修改；ISR 也发布 bike 导出请求。当前无 FreeRTOS 内核和任务。
