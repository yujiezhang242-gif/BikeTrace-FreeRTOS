# 内存基线

来源：包内 MDK-ARM/BikeTrace/BikeTrace.map，历史数据，非本次构建。

|项|字节|
|---|---:|
|Code|44208|
|RO|2828|
|RW|2420|
|ZI|13188|
|Code+RO|47036|
|map Total ROM Size（含压缩装载数据）|48112|
|RAM RW+ZI|15608|
|20KiB RAM 余量|4872|

startup_stm32f103xb.s：MSP 栈 0x800=2048B，C heap 0x1600=5632B。
map 已将 HEAP(0x20001ef8,5632B)、STACK(0x200034f8,2048B) 计入 RAM，不能再重复扣除。
当前 RAM 中其余占用为 7928B。这个余量不是可全分配给任务的安全预算。

FreeRTOS heap、TCB、任务栈、队列：尚未添加；运行高水位和最低堆余量 NOT_RUN。P1 必须根据实际参与链接的对象重新预算；不能在旧15608B基础上机械添加全部新堆，也不能暗中缩小关键栈。
