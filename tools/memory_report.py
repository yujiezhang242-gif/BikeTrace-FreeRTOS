"""Report Cortex-M3 allocated object sections, not a linked Keil memory claim."""
from pathlib import Path
import struct,json
root=Path(__file__).resolve().parents[1];rows=[]
for profile in ['lite','heartbeat','isolated']:
    sums=[0,0,0];objects=[]
    for p in sorted((root/'tests/build/cross'/profile).glob('*.o')):
        b=p.read_bytes();h=struct.unpack_from('<16sHHIIIIIHHHHHH',b);ram=0
        for i in range(h[12]):
            _,typ,flags,a,o,size,link,info,align,ent=struct.unpack_from('<IIIIIIIIII',b,h[6]+i*h[11])
            if flags&2:
                sums[0 if flags&1 else 1 if flags&4 else 2]+=size
                if flags&1:ram+=size
        if ram:objects.append({'object':p.name,'ram_sections':ram})
    rows.append({'profile':profile,'unlinked_ram_sections':sums[0],'unlinked_code_sections':sums[1],'unlinked_ro_sections':sums[2],'msp_reserved':2048,'ram_remaining_upper_bound_check':20480-sums[0]-2048,'objects':objects})
(root/'docs/evidence/object_memory.json').write_text(json.dumps(rows,indent=2)+'\n')
text='''# C8 内存预算与边界

ARMCC5最终Code/RO/RW/ZI、map：NOT_RUN。以下是Clang Cortex-M3对象节之和，未链接、未删除无用函数、未加入C库和启动向量，不能当作最终固件大小或Keil门禁。核对目标仍是64KiB Flash / 20KiB RAM。

|检查配置|未链接RAM节合计|另预留MSP|20KiB差额|未链接Code节|未链接RO节|
|---|---:|---:|---:|---:|---:|
'''
for row in rows:text+='|{profile}|{unlinked_ram_sections}|{msp_reserved}|{ram_remaining_upper_bound_check}|{unlinked_code_sections}|{unlinked_ro_sections}|\n'.format(**row)
text+='''
上述Code/RO含大量未引用HAL/内核函数；最终linker会移除，C库也会增加占用。没有ARMCC5链接结果不能判断Flash最终是否通过。RAM对象估算仅作提前发现超限，ARMCC5枚举/对齐/优化可能不同。

FreeRTOS heap_4静态10240B，已经包含在RAM节中，不能再次相加。旧C heap从5632B改为0：新增业务不调用malloc/free，固定JSON workspace替代cJSON；格式化/数学库的实际ARM链接依赖仍须Keil确认。MSP仍2048B，没有通过削减主栈掩盖空间不足。

任务栈：BikeGps384word=1536B；StorageNet768word=3072B；UiSensor384word=1536B；Idle128word=512B。总6656B，从RTOS heap分配。P1两心跳各192word，另有Idle栈。

按当前32位对象layout（TCB80B、queue72B、stream32B、StorageRequest64B、Telemetry96B）估算，含任务/堆块头、三个stream、8个queue和I²C mutex的启动分配约9352B，heap可用余量约880B。此数字不是硬件测量；64位host测试大小不可直接替代C8。

运行时读取app_health.heap_free/heap_min和control_stack/service_stack/ui_stack（word）。任务创建或分配失败进入hook；没有新地图/JSON动态分配。最终门禁仍需Keil map、60min高水位和最大中断嵌套实测。完整日志见evidence/object_memory.json。
'''
(root/'docs/memory_budget.md').write_text(text)
(root/'docs/rtos_memory.md').write_text('# RTOS内存入口\n\n见memory_budget.md。堆包含任务栈、TCB和IPC，禁止重复记账；运行指标尚未测得。\n')
print('Memory object budget written; Keil linked size NOT_RUN')
