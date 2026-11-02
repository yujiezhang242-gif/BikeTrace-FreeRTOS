# BikeTrace C8 LITE 最终代码架构

本轮实施 P1–P7 的 C8 精简代码路径；不是 RCT6 FULL 五任务固件，不代表已通过实物验收。目标、启动文件和内存边界仍为原 STM32F103C8。

## 任务及所有权

|任务|优先级/栈（32位word）|唯一拥有的状态|运行方式|
|---|---|---|---|
|BikeGps|3 / 384|BikeCore、GpsFix、按键状态、骑行统计|20ms周期，GPS增量解析、控制队列、实际经过时间积分|
|StorageNet|2 / 768|StorageService、Flash SPI访问、只读历史扫描、NetworkService、AT、NavigationCore、调试命令|10ms周期；每轮最多256B网络输入，存储FIFO和有界状态机|
|UiSensor|1 / 384|OLED、AHT21、可选且明确确认的HMC5883L采集|50ms运行，OLED200ms，AHT触发与读取分离，设备缺失降级|
|Idle|0 / 128|RTOS空闲资源回收|内核|

C8合并了设计的GPS/Bike、Storage/Network、UI/Sensor职责，避免五任务额外栈。SPI仍只有一个任务访问；总线mutex协调UI与StorageNet的EEPROM只读事务。不能因此推断已验证实时性能。

Bike状态：IDLE → RIDE → STOP_PENDING → STOP_COMMITTED；错误转STOP_ERROR，按键可重试。只有匹配request_id的成功ACK能够提交STOP。恢复是显式只读请求，要求IDLE和新鲜GPS；不自动恢复或回写旧数据。

GPS ISR只写StreamBuffer并重启单字节接收，GpsTask职责由BikeGps执行。值对象经队列复制；没有跨任务传播GPS局部字符串。UART1命令在StorageNet解释，clear data/normal mode均拒绝写入。

## 消息与溢出

- 控制队列4项，与GPS/UI独立。失败可观测。
- Storage FIFO 5项，普通采样只使用4项，保留STOP槽；STOP和所有先前入队样本保持顺序，不能overwrite。
- 缓存满13项或STOP后编码256B；程序/读回/检查点失败均保留缓存。后续不能接受的样本返回错误并计数，不能声称已保存。
- UI、网络遥测、传感器、导航用长度1值队列保留最新快照；不积累离线上传请求。
- GPS/网络/调试输入分别256/512/64B StreamBuffer，满时计数。网络溢出丢弃受损帧并转退避，GPS等待下一个$重新组帧。

## 外设与协议

TIM4唯一提供HAL毫秒时基；FreeRTOS SysTick独立负责调度。业务不再启用TIM2秒标志。原main的测试里程、阻塞网络初始化、磁校准均不在新启动路径。

默认网络关闭，地址空。可配置的EC600N有限状态机沿用原QICSGP/QIACT/QIOPEN透传流程；超时和退避有界。保留time request/response、path_plan、request_path_plan、upload data命令和原data.paths字段；上传保留GCJ-02转换、单位和state=0/1/2。固定容量JSON解析器替换动态cJSON，最多2048B/16 steps/144 tokens/12层，不分配JSON堆对象。路线先验证再整体替换。

软件I²C开漏释放SDA，处理SCL拉低超时/NACK，所有出口STOP并释放mutex；不持锁等待网络、存储ACK或AHT转换。OLED缺失5秒后重试；AHT失败不阻止其他任务。磁力计默认不访问，确认HMC且身份H43匹配后才允许可选驱动；没有校准精度承诺。

## 持久化

默认所有旧Flash/EEPROM只读。Legacy reader独立于旧的有写副作用init_index，能识别索引第0页，保留76B索引、256B历史页解码及按表边界导出。

真实写后端是可选isolated_flash：必须明确批准、位于0x200000之外、容量/4KB边界校验通过的独立区域。数据仍使用legacy 256B codec，最后4KB为带CRC与最终提交标记的只增检查点日志；没有任何擦除API。部分提交槽不能覆盖，日志满返回APP_FULL。不会写旧EEPROM标志和历史索引。

真实分区未提供，所以保持writes_enabled=0；默认STOP显示保存受保护错误。隔离区恢复扫描只读，不能将“读到检查点”当作已完成断电实物测试。

AHT21初始化参数根据[厂商A1手册](https://aosong.com/userfiles/files/media/AHT21%20%E8%8B%B1%E6%96%87%E7%89%88%E8%AF%B4%E6%98%8E%E4%B9%A6%20A1%2020201222.pdf)使用BE 08 00，等待时间在解锁后通过下一轮任务实现；实物模块版本仍需核实。
