# BikeTrace FreeRTOS：Codex 分阶段开发任务（可直接复制）

> 用法：把原始 BikeTrace 工程与《BikeTrace_FreeRTOS_重构设计说明书.md》、`AGENTS.md` 放在同一 Codex 工作区。**一次只发送一个阶段的指令**。每阶段完成后，在自己的 VMware Windows 中用 Keil 重新构建，反馈错误和运行结果，再发下一阶段。未实测不能填 PASS。

## 全阶段通用约束

1. 首先阅读 `AGENTS.md` 和设计说明书，扫描实际源码，对不一致处引用源文件和函数名；源代码优于文档假设。
2. 不能覆盖或删除原始可编译工程；先创建工作副本/分支。所有新增功能先用模拟接口或 host test 验证。
3. 未获得许可不得执行 W25Qxx 整片/扇区擦除、EEPROM 清空、连接旧服务器或修改用户历史记录。
4. **技术栈固定 STM32 HAL + C + FreeRTOS**，兼容原 ARM Compiler 5；实际硬件芯片型号不确定时不得擅自切换目标。
5. 每次结尾列出：新增/修改文件、主要逻辑、构建状态及日志、host tests、实物测试状态、Code/RO/RW/ZI、任务栈/堆使用、已知风险、下一阶段待办。
6. 不得声称完成第 N 阶段以外的功能；未运行的步骤必须标 `NOT_RUN`，并给出在 Keil 中具体复现步骤。

---

## P0：原工程审计与不破坏现状的基线

**发送给 Codex：**

> 只执行 BikeTrace FreeRTOS 设计说明书 P0。逐个核对 `main.c`、`bike.c`、`network.c`、`bsp_uart_driver.c`、`bsp_gps_driver.c`、`flash_data.c`、`flash_index.c`、`restore.c`、`usart.c`、`stm32f1xx_it.c`、`.ioc` 和 `.uvprojx`。在复制工程中建立 git 基线或等价的可恢复快照。输出完整源文件清单、已有编译器/芯片配置、源代码当前启用/注释的业务调用链、原记录格式、外设引脚和风险清单。不得修改功能和存储数据。创建 `docs/baseline_report.md`、`docs/target_config.md`、`docs/memory_baseline.md`、`docs/architecture_current.md`。如果无法使用 Keil 运行编译，标记 `BUILD_NOT_RUN` 并向我提供 Windows Keil Rebuild 操作方法。完成后停止。

**通过门禁：** 原工程源码未被覆盖；基线 map/AXF 和关键问题可追溯；MCU 实物型号待核实时有 `UNCONFIRMED` 标记。

## P1：FreeRTOS 最小启动与两任务调度

**发送给 Codex：**

> 只执行 P1。在已复制的工程里集成版本明确、兼容 Cortex-M3 与 ARMCC5 的 FreeRTOS kernel/port；若无法获取可信内核源码，不伪造，请说明依赖和需要我放置的位置。生成 `FreeRTOSConfig.h`，定义最低限度的 stack overflow、malloc fail、assert、heap 监控。明确 SysTick、SVC、PendSV 和 HAL timebase 的唯一责任者，排除中断入口重复定义。创建两个只修改 volatile 计数器的周期任务（500ms 与 1000ms），不要在任务里触发 GPS/EC600N/Flash/OLED 原业务初始化。若 C8 内存放不下，先输出内存预算及具体超额项，不要通过悄悄减小关键栈来掩盖错误。更新 `.uvprojx` 让所有新增源文件确实参与编译。提交 `docs/build_report_P1.md`、`docs/irq_vector_map.md`、`docs/rtos_memory.md` 和 Keil 断点操作说明。停止在 P1。

**通过门禁：** 可构建且硬件调试时两任务计数合理；无重复 SysTick/PendSV/SVC；内存报告清晰。无硬件时仅记录构建结果。

## P2：GPS 串口中断与安全 NMEA 解析

**发送给 Codex：**

> 只执行 P2。保持 USART3 9600 8N1 及其 HAL 回调入口；将 `HAL_UART_RxCpltCallback()` 中 GPS 字节处理改为常量时间、有界缓冲或 StreamBuffer `FromISR` + GpsTask 解析；接收后正确重启下一字节接收。检查 FreeRTOS 可调用的 NVIC 优先级，设置正确的 syscall 中断范围。抽取可在 PC 上独立测试的 NMEA RMC parser，支持 `$GPRMC`/适用时 `$GNRMC`、XOR checksum、CRLF、超长丢弃、有效性 A/V、纬经方向、时间戳及过期检测。GpsTask 发布固定大小的 GpsFix 值对象，不向其他任务传播可失效的局部字符串指针。USART1 原 `cmd_revevied()` 16 字节越界及 ISR 擦除命令只能修复/禁用，不能继续保留破坏性入口。提交 1000 帧压力样例与坏帧测试，更新 Keil 报告，停止。

**通过门禁：** checksum/边界/拆帧和超时测试通过；没有 ISR 调用 `printf/HAL_Delay/Flash erase`；GPS 丢字节/队列满有计数。

## P3：Bike 状态机与实时骑行统计

**发送给 Codex：**

> 只执行 P3。提取可脱离硬件测试的 `BikeCore`，保留 IDLE/RIDE/STOP 和 `NOT_RESTORE` / `IS_RESTORE` 语义；设计中可以增加 STOP_PENDING/STOP_COMMITTED 内部状态。BikeTask 成为唯一可变 BikeState 写者，其他任务只能通过请求事件和快照通信。将 `bike_handle()` 中已注释的按键及周期更新以 RTOS 事件机制恢复，不允许继续让 `main()` 的 `test(time_str, dis_km)` 抢占定时标志。20ms 按键扫描；使用实际单调经过时间统计秒数与里程，拒绝无效/过期 GPS；修复平均速度算法、零分母和数据缩放单位。网络/存储仅发布异步请求，不得在 BikeTask 同步等待连接或 Flash 擦除。提交 host 测试：10kn→18.52km/h、1km/600s=6km/h、无效 GPS、重复按键、周期延迟。停止。

**通过门禁：** 纯业务状态/计算测试可运行，原业务的状态机结果可解释；没有多个任务直接写 `bike`。

## P4：StorageTask、轨迹记录与断电恢复

**发送给 Codex：**

> 只执行 P4。保持原 `13×18 + 2 + 20 = 256B` legacy 格式可读取；新增 host 端 storage codec 验证字段偏移和单位缩放。StorageTask 独占 W25Qxx SPI/AT24C02 写与索引操作，BikeTask 只通过 FIFO 消息发送 SAMPLE 和 STOP_FLUSH，队列满必须显示错误。对页写入加地址/范围保护、擦除前判断、读回校验、结果码；仅在成功后清空待写缓存并发布带 request_id 的 StorageAck。停止骑行时保证剩余轨迹 flush 完成后才标记成功。默认禁止写原 Flash 历史区，需单独配置测试区域且经我确认。完成 `restore.c` 只读恢复分析以及异常场景 host test，仍不得执行真实擦除。停止。

**通过门禁：** 满13条生成1页；提前 STOP flush；写失败不假成功；legacy 解码与索引不回归。

## P5：NetworkTask、EC600N、JSON 与云端导航

**发送给 Codex：**

> 只执行 P5。NetworkTask 独占 USART2 的 AT 命令/URC/TCP 状态机；原 `at_wait_for_response()` 不得继续无限忙等。用接收事件和有界超时替换，设置断线退避，不影响 BikeTask 的本地计时。实现受控 TCP 消息成帧；优先兼容旧服务端的实际协议，不能擅自要求服务端更改。测试 JSON 拆包、粘包、坏 JSON、连续多消息、2KB 以上拒绝、路径 steps 限额、cJSON 对象释放。时间响应、路径规划完成通过消息或不可变快照通知 Bike/UI。旧公网服务器未确认前只能 mock，不实际连接。提交测试与内存报告，停止。

**通过门禁：** 断网模拟不影响本地采样；没有未界定长度的 JSON 解析；AT 事务归属单一任务。

## P6：OLED、软件 I²C、传感器和整体协作

**发送给 Codex：**

> 只执行 P6。UiTask 独占 OLED 绘制，快照队列长度为1，过时 UI 帧可覆盖；基于 PB6/PB7 的共享软件 I²C 建立互斥或执行器，检查原 `iic.c` GPIO push-pull 与释放 SDA/ACK 电气时序。AHT21、HMC5883L、AT24C02 和 OLED 访问同一总线时必须保证串行化和失败超时；任何任务持 I²C 锁时不能等待网络或存储 ACK。磁力计真实型号未核实时不得假定 HMC5883L 驱动适用于 QMC5883L。实现缺屏/传感器 NACK 的降级运行和指标；为 VMware Windows Keil 给出断点/观察变量和外设联调顺序。停止。

**通过门禁：** 屏幕切页无卡死；断网仍能本地刷新；I²C 失败不永久持锁；日志含设备错误码。

## P7：资源评估、故障恢复与最终交付

**发送给 Codex：**

> 只执行 P7。汇总所有任务栈高水位、FreeRTOS heap 空闲与最低余量、链接 map 静态占用、队列溢出、GPS 无效帧、Flash 写入校验、网络重试指标；进行 host regression 和可执行的压力测试。准备 60min 正常运行、断网30min、20次重启/断电演练的实物测试清单；没有实际做过的项写 NOT_TESTED，不能虚构 PASS。产出 `docs/architecture_final.md`、`docs/irq_vector_map.md`、`docs/memory_budget.md`、`docs/verification_report.md`、`docs/user_guide.md`、`docs/known_issues.md` 和完整 Keil 工程。旧 Flash 数据可读是硬性要求，保留回滚方案。最后给我一份面试可解释的具体改造记录，但不得杜撰实测性能。

**通过门禁：** 各阶段 Gate 报告齐全；Keil 构建和硬件验证证据清楚；数据/任务/异常一致性可复现。

---

## 用户每次返回给 Codex 的反馈模板

```text
当前阶段：P__
Windows Keil 编译结果：__ Errors / __ Warnings
Build Output 最早错误：___
目标 MCU 实物型号：___（如未核实写 UNCONFIRMED）
是否上板：是/否；下载器：ST-Link/J-Link
运行现象（Watch/串口日志）：___
数据擦写是否许可：否/仅独立测试分区（写清地址）
请仅根据这些结果修正本阶段，不要提前开展下一阶段。
```
