# 本轮交付与验收状态

2026-11-02。用户授权连续推进，本轮已实施P1–P7的C8 LITE代码和交接文档，未按设计文档内复制的阶段指令反复停顿。以下严格区分代码、本机验证与实物放行。

## 交付

- 新工程：BikeTrace_FreeRTOS/MDK-ARM/BikeTrace_FreeRTOS.uvprojx，含BikeTrace_C8_LITE与BikeTrace_P1_Heartbeat两个target，各49个源文件（含18个新增应用C单元）。
- FreeRTOS V10.6.2，RVDS Cortex-M3 port、heap_4及完整license/provenance。
- 原344文件的完整可恢复归档及SHA-256，原下载目录未修改。
- 已修改/新增固件文件43个（不含vendor内核），见changed_files.md/changes.patch；Keil工程源清单审计通过。

## 已运行结果

|检查|结果|证据|
|---|---|---|
|原始归档与344文件hash|PASS|tools/verify_baseline.py|
|Host业务断言|PASS，1327项|evidence/host_tests.txt|
|ASan/UBSan|PASS，错误即退出|同上，run_host_tests.py|
|真实FreeRTOS queue/stream/heap API|PASS，单线程test port|evidence/rtos_resource_tests.txt|
|Cortex-M3 C对象默认LITE|PASS，0 errors / 0 warnings|evidence/clang_cortex_m3_lite.txt|
|Cortex-M3 C对象P1心跳|PASS，0 errors / 0 warnings|evidence/clang_cortex_m3_heartbeat.txt|
|Cortex-M3 C对象隔离写入/可选HMC条件分支|PASS，0 errors / 0 warnings；未执行设备操作|evidence/clang_cortex_m3_isolated.txt|
|Keil源路径、单port/heap、ISR默认保护|PASS|evidence/project_audit.txt|
|ARMCC5完整构建/链接/RVDS汇编|BUILD_NOT_RUN|当前Mac无Keil/armcc|
|新AXF、Keil map、Code/RO/RW/ZI|NOT_RUN|旧包产物不是本轮固件|
|实物调度/栈高水位/最低heap|NOT_RUN|必须在目标板测量|
|真实Flash写入、云连接、断电|NOT_TESTED|默认关闭，未做任何真实擦写|

Clang检查替换为官方GCC port以验证ARM C集成；.uvprojx仍使用官方RVDS port。测试port不模拟真实调度或中断嵌套，所以不能声称Keil或硬件通过。

## 覆盖

T01/T02：1000个RMC帧、坏校验和、CRLF、超长、日期、A/V（含无时间V）、方向、时间回绕。
T03–T05：10kn=18.52km/h、1km/600s=6km/h、实际2.5秒延迟、零分母、GPS过期不延用、状态/重复按键/错误与重复ACK、恢复幂等拒绝、30min断网的纯时间输入模拟。
T06–T08：独立Python生成的256B合成golden、精确字段偏移/单位、13条分页、提前STOP、写/回读/检查点失败、保留缓存、重试不重复、索引第0页、非法恢复范围；隔离区域保护、提交标记前中断、20次不改写的RAM恢复扫描、日志满。
T09/T10：字节拆包、10个粘连消息、坏JSON/长度、16/17步限制、原子路线替换、5000组随机输入、AT超时/退避/URC、UTC时间推进、上传缓冲长度保护。
Navigation：跨多路段推进、终点、重新骑行重置、100m方向偏离触发重规划。
AHT：CRC、busy拒绝、25.00℃/50.00%解码。
T11：重复START/数据位读取、设备NACK、SCL卡低超时、STOP和mutex释放。
T12：实际内核FIFO与STOP预留槽、UI最新覆盖、FromISR stream满、heap耗尽及释放恢复；不验证硬件调度。

## Gate与未完成验收

P0审计和代码快照已完成；P1–P7代码路径已实现并完成可运行的本机检查。所有要求真实Keil构建、芯片确认或实物的门禁仍NOT_VERIFIED，不能整体标PASS。当前工程是待Keil/上板验收的代码交付，不能称为可直接投产固件。

C8对象RAM预算约1KB外部余量，见memory_budget.md。真实写入保持PROTECTED、网络默认OFF、磁力计默认UNCONFIRMED；这些保护没有因本机mock通过而打开。隔离写入不改旧EEPROM和索引，这是保留历史数据的C8实现策略。

具体Windows操作见user_guide.md；完整待测矩阵见hardware_test_plan.md；风险与限制见known_issues.md；面试可用事实见interview_change_log.md。

## 复现命令

```sh
python3 tools/verify_baseline.py
python3 tools/run_host_tests.py
python3 tools/run_rtos_tests.py
python3 tools/generate_project.py
python3 tools/project_audit.py
python3 tools/run_cross_check.py
BIKETRACE_CHECK_PROFILE=heartbeat python3 tools/run_cross_check.py
BIKETRACE_CHECK_PROFILE=isolated python3 tools/run_cross_check.py
python3 tools/memory_report.py
```
