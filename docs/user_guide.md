# 使用与Keil验证

## 打开新工程

将BikeTrace_FreeRTOS整个目录复制到Windows本地路径，打开MDK-ARM/BikeTrace_FreeRTOS.uvprojx。不要打开旧BikeTrace.uvprojx作为新固件入口。

选择BikeTrace_P1_Heartbeat先验证内核，再选择BikeTrace_C8_LITE。保持STM32F103C8、ARMCC5.06u6、64KiB ROM/20KiB SRAM；核对实际芯片丝印和HSE。原.ioc仅作为裸机引脚参考，不能直接CubeMX覆盖本次自定义boot、IRQ和TIM4实现。

执行Rebuild all target files，保存完整Build Output、map、AXF至docs/evidence/windows/。本轮没有生成新的Keil AXF，不要把包内BikeTrace/BikeTrace.axf当作新固件。先解决第一条错误，不使用旧增量日志作为新构建证据。

## Keil断点与观察

1. P1：在App_Start、vTaskStartScheduler、Heartbeat500、Heartbeat1000设断点；开始后移除周期断点连续运行，观察app_health.heartbeat_500约为heartbeat_1000两倍。暂停调试会影响计时，不能据断点次数作实时性能结论。
2. 同时观察uwTick、xTickCount，确认HAL/RTOS各自递增；检查TIM4和SysTick入口，SVC/PendSV没有重复符号。
3. 观察App_Assert、vApplicationMallocFailedHook、vApplicationStackOverflowHook；fault和assert_file/line保留首类故障。栈高水位单位word，heap单位字节。
4. LITE先只接GPS/按键，观察gps_good、gps_bad、gps_rx_overflow、control_cycles和最大周期延迟。GPS未连接时不允许开始新骑行。短按PC14启停/重试，PB0切三页。
5. 再接OLED/AHT21，共线外部上拉与电平需实测。缺屏允许继续计时；OLED报错5秒重试。磁力计默认关闭，不能假定QMC兼容HMC。
6. 最后验证SPI只读JEDEC、历史导出。不要执行旧clear data；新命令已拒绝。原历史初始化不在新启动链。

## UART1调试命令

115200 8N1，换行结束，最多47字符，超长整行丢弃。

|命令|行为|
|---|---|
|health|空闲/最低heap、三个任务栈余量、GPS/队列指标|
|get data|只读扫描索引，返回表数、版本、JEDEC|
|export N|按历史表边界逐页导出原始度分坐标、速度和汇总；不会因8位页号回绕|
|restore|只读EEPROM和最后历史页；排队请求恢复，要求IDLE和有效新鲜GPS；不恢复物理写权限|
|start stop|与实体启停键同一控制队列|
|screen|与实体切页键同一控制队列|
|clear data / normal mode|返回PROTECTED，不擦除、不改标志|

默认屏幕STORE RO表示存储只读；STOP转SAVE ERROR/LOCK是预期保护行为，不能当作已保存。没有默认成功的RAM模拟固件记录。

## 可选配置

app_config.h集中配置。网络默认APP_NETWORK_ENABLED=0，HOST空、PORT=0；只有确认模块和服务器后填写新确认的endpoint。不要重新使用源码旧IP。真实联网验收之前先使用host mock。

物理写入必须有APP_STORAGE_WRITE_ENABLED=1、APP_FLASH_REGION_APPROVED=1和明确START/END，区域应在[0x200000,实际JEDEC容量)内，4KB对齐且至少8KB。此轮没有分区批准，因此全部保持0。实现不擦除区域；非空不兼容页/日志用尽会返回错误，不自动格式化。新隔离区使用检查点日志而非改动旧EEPROM/索引。

磁力计APP_MAGNETOMETER_HMC_CONFIRMED默认0，确认真实HMC5883L后方可开启；身份不匹配仍降级。原校准数据未自动改写，方向精度必须实测。

## 回滚

运行python3 tools/verify_baseline.py核验归档；把baseline/BikeTrace_original.tar.gz解压到全新的空目录，再使用其中原MDK-ARM/BikeTrace.uvprojx。禁止解压覆盖当前改造成果。原下载目录也保持不变。回滚固件有原启动写入副作用，不应在未备份历史数据时随意下载运行。
