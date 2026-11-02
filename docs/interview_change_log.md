# 可解释的改造记录（不含虚构实测）

- 识别原main模拟里程消费TIM2标志、bike_handle禁用按键/统计的问题，改为单一BikeCore写者、20ms按键扫描和单调时间差统计。10kn=18.52km/h、1km/600s=6km/h由host测试验证。
- 将UART ISR中的命令解释/擦除和GPS字符串处理替换为有界StreamBuffer；GPS解析带checksum、CRLF、A/V、方向及过期判断，GPS修复对象按值传递。
- 在C8资源约束下选三任务LITE，保留两心跳调试target；明确SysTick属于内核、TIM4属于HAL，NVIC优先级与FromISR范围一致。
- 将存储改为异步FIFO、STOP保留槽、request_id ACK、读回验证和失败缓存保留；保持legacy 256B解码；新增独立区域CRC提交日志，默认禁止历史区写入。
- 网络有界AT事务/退避与JSON拆包粘包，固定容量解析器取消JSON动态堆使用；路线成功后整体替换，控制路径不等待联网。
- 软件I²C有mutex、开漏、NACK/SCL超时和所有出口释放；AHT转换期间不持锁，缺设备可降级。提供栈/堆/溢出/失败计数。

可引用的结果是本仓库测试日志中的host断言、sanitizer、真实内核资源API测试和Clang Cortex-M3对象编译。不能写“上板零故障”“实时性提高X%”“稳定运行60分钟”“已通过20次断电”，也不能把Clang结果写成Keil编译通过。
