# C8 内存预算与边界

ARMCC5最终Code/RO/RW/ZI、map：NOT_RUN。以下是Clang Cortex-M3对象节之和，未链接、未删除无用函数、未加入C库和启动向量，不能当作最终固件大小或Keil门禁。核对目标仍是64KiB Flash / 20KiB RAM。

|检查配置|未链接RAM节合计|另预留MSP|20KiB差额|未链接Code节|未链接RO节|
|---|---:|---:|---:|---:|---:|
|lite|17397|2048|1035|72412|9075|
|heartbeat|12009|2048|6423|67656|8113|
|isolated|17450|2048|982|73228|9084|

上述Code/RO含大量未引用HAL/内核函数；最终linker会移除，C库也会增加占用。没有ARMCC5链接结果不能判断Flash最终是否通过。RAM对象估算仅作提前发现超限，ARMCC5枚举/对齐/优化可能不同。

FreeRTOS heap_4静态10240B，已经包含在RAM节中，不能再次相加。旧C heap从5632B改为0：新增业务不调用malloc/free，固定JSON workspace替代cJSON；格式化/数学库的实际ARM链接依赖仍须Keil确认。MSP仍2048B，没有通过削减主栈掩盖空间不足。

任务栈：BikeGps384word=1536B；StorageNet768word=3072B；UiSensor384word=1536B；Idle128word=512B。总6656B，从RTOS heap分配。P1两心跳各192word，另有Idle栈。

按当前32位对象layout（TCB80B、queue72B、stream32B、StorageRequest64B、Telemetry96B）估算，含任务/堆块头、三个stream、8个queue和I²C mutex的启动分配约9352B，heap可用余量约880B。此数字不是硬件测量；64位host测试大小不可直接替代C8。

运行时读取app_health.heap_free/heap_min和control_stack/service_stack/ui_stack（word）。任务创建或分配失败进入hook；没有新地图/JSON动态分配。最终门禁仍需Keil map、60min高水位和最大中断嵌套实测。完整日志见evidence/object_memory.json。
