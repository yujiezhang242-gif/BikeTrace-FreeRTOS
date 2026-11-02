# 待执行实物验收（全部初始NOT_TESTED）

|项目|方法|需保存证据|状态|
|---|---|---|---|
|P1两任务|连续观察500ms/1000ms计数、HAL tick、内核tick|Watch截图/计数时间区间|NOT_TESTED|
|60min正常运行|有效GPS、按键切页、传感器接入；每分钟读health|栈最低余量、heap min、最大控制周期、错误计数|NOT_TESTED|
|30min断网|运行中断开模块网络，观察本地elapsed和distance，恢复后检查退避|前后快照、重试和队列计数|NOT_TESTED|
|缺屏/传感器NACK|断电后移除设备，再开机；运行总线故障按电气规范操作|错误码、仍可启停的证据|NOT_TESTED|
|GPS坏帧/掉线|串口注入坏checksum、超长、A/V和停止输入|无虚假距离、溢出与过期状态|NOT_TESTED|
|历史读取|先备份真实原页，导出多个表，比较解码内容|原始dump/哈希/导出结果|NOT_TESTED|
|隔离区保存|仅经确认的独立区域；13条、提前STOP、读回比较|实际地址/JEDEC/逐页hash/ACK|NOT_TESTED|
|20次重启/断电|分别在页编程、回读、检查点payload/marker阶段断电，只用测试区|每轮时点、恢复扫描、记录与ACK一致性|NOT_TESTED|
|资源极限|路线16/17步、满帧、队列压力；观察assert/溢出|map、高水位、最低heap、错误计数|NOT_TESTED|

每次构建保留编译器版本/第一条错误/warning总数。未完成实物数据备份和分区确认时，不执行任何写入/断电存储测试。
