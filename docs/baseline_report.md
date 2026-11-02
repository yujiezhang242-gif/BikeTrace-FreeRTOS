> 本报告为P0时点记录；后续P1–P7代码交付状态见verification_report.md。

# P0 原工程审计（2026-11-02）

原目录：`/Users/zyj/Downloads/BikeTrace_Path_Reload - 简历包装`，只读复制；工作副本：`BikeTrace_FreeRTOS/`。344 个文件的 SHA-256 存于 baseline_manifest.json，包括旧 map、AXF 和编译日志。未发现 AGENTS.md。本阶段未修改固件源码、工程配置或任何设备数据。

## 证据与状态

- 当前环境 BUILD_NOT_RUN：Mac 没有 armcc / UV4；不能用 clang 代替 Keil 构建验收。
- 包内历史日志 MDK-ARM/BikeTrace/BikeTrace.build_log.htm 为增量编译（只编译 main.c），结果 0 Errors / 1 Warning。设计文档所述 12 Warnings 不能作为本次 Rebuild 的证据。
- 历史 map 和 AXF 已保留；不能证明产物与全部当前源码一一对应。
- Host：复制完整性检查 PASS；业务测试 NOT_RUN（本阶段不修改功能）。
- 硬件 HARDWARE_NOT_VERIFIED；芯片丝印与板卡 UNCONFIRMED。

## 风险清单（以源码为准）

|文件/函数|发现|后续阶段|
|---|---|---|
|Core/Src/main.c::main|正式循环启用 test(time_str, dis_km)，每 tick 模拟增加 0.01km；MX_DMA_Init 注释|P1/P3|
|APP/src/bike.c::bike_handle|按键及 bike_update 注释，云接收/导出启用|P3|
|APP/src/bike.c::bike_init|同步网络初始化、Flash 索引初始化；main 还校准磁力计，不可原样作为安全 RTOS 启动|P1|
|BSP/src/bsp_uart_driver.c::cmd_revevied|16B 缓冲先写后检查、换行再写终止符，越界；clear data 在 ISR 整片擦除 Flash/EEPROM；normal mode 也改 EEPROM|P2|
|BSP/src/bsp_gps_driver.c::rec_gps_data_to_get_gprmc|uint8_t index 回绕；256B 接收向128B解析复制无界；ISR/main 共享帧，无 checksum/过期判断|P2|
|APP/src/utils.c::calc_average_speed|km*1000/sec/3.6 错误且无零分母保护；应 km*3600/sec|P3|
|APP/src/flash_data.c::save_item/write_record_to_flash|失败仍清缓存、更新计数；STOP 同步写且 delay；无分区上界/回读验证|P4|
|APP/src/flash_data.c::print_table_n|npage 为 uint8_t，超过255页会回绕，导出不能保证终止|P4|
|APP/src/flash_index.c::serch_valid_page_index/read_index_from_flash|0 同时表示无有效页和有效第0页；不能可靠区分出厂状态|P4|
|APP/src/flash_index.c::set_next_index/init_index|开机 read_index_from_flash 可调用 set_next_index 并在轮转时擦索引；init_index 还清 EEPROM 记录数；读取初始化并非只读|P4|
|Core/Inc/main.h 与 APP/src/flash_index.c|PAGE_SIZE 分别256/16，宏重定义；restore.c 实际通过 main.h 使用256，不能误报恢复步长必然是16|P4|
|APP/src/restore.c::get_last_ride_record_addr/restore_last_ride_status|记录数0未拦截，地址下溢；读取返回值、记录标志与分区界限未完整校验；恢复直接写 bike|P4|
|APP/src/network.c::bike_cloud_recv_data|recv 后才按超时重置 total_len，可能破坏新片段；无明确多 JSON 成帧|P5|
|APP/src/network.c::parse_json_path_plan|steps 无上限；直接替换现有指针；失败 free 后残留指针，需原子替换审计|P5|
|BSP/src/at_ec600n_handler.c::at_wait_for_response|512B栈缓冲，忙等；timeout=-1 可无限等待|P5|
|Core/Src/usart.c|三个 UART NVIC 均0，不能直接调用 RTOS FromISR API|P2|
|Core/Src/gpio.c / Core/Src/iic.c|共线软件I²C、push-pull配置，需电气时序和失败超时审计|P6|

## Windows Keil 重建

1. 将工作副本复制到 Windows 本地磁盘，例如 C:\BikeTrace_FreeRTOS；打开 MDK-ARM/BikeTrace.uvprojx。
2. Options for Target 核对 STM32F103C8、ARM Compiler 5.06u6、ROM 0x08000000/0x10000、RAM 0x20000000/0x5000；核对 CMSIS/DFP 依赖。
3. Project → Rebuild all target files，保存完整 Build Output 到 docs/evidence/P0_rebuild.txt；不要直接下载本基线到设备（存在启动写入路径）。
4. 保存新 map/AXF；记录所有 warning、Code/RO/RW/ZI 与编译器版本。旧日志不能覆盖新证据。

## 下一步

P1：保持 C8 目标，隔离原业务启动，接入可信 ARMCC5 Cortex-M3 FreeRTOS 内核、独立 HAL 时基、两周期计数任务；先完成内存预算。用户已授权常规逐步改造，无需逐阶段请求许可；实物型号、真实 Keil 和存储测试分区仍必须有事实证据。P0 审计已交付，构建/上板门禁未验证，P1–P7 未实施。

## 可恢复快照

当前环境禁止写工作区 .git/index.lock，未创建 commit。使用等价归档 baseline/BikeTrace_original.tar.gz 和 SHA256SUMS 保留原始完整工程（含旧产物）；逐文件清单见 baseline_manifest.json。后续修改不应更新此归档。恢复时解压到新的目录，不覆盖现有工作成果。
