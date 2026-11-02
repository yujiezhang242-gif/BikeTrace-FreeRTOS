# Legacy 存储布局（只读审计）

W25Qxx：索引扇区0 [0x0000,0x1000)，扇区1 [0x1000,0x2000)。16 个表起点：0x2000、0x21000、0x40000、0x60000、0x80000、0xA0000、0xC0000、0xE0000、0x100000、0x120000、0x140000、0x160000、0x180000、0x1A0000、0x1C0000、0x1E0000；源码设计终点0x200000。此为布局，不能证明物理 Flash 容量。

index 76B（ARM ABI预期）：ntables@0 u16、next_table_i@2 u16、table_ts[16]@4、next_version@68 u32、next_page_i@72 u16、next_no@74 u8、write_flag@75 u8。每索引占用一页256B，单扇区16页。

记录256B：13个18B item，占[0,234)；保留[234,236)；status[236,256)。item：lon double@0，lat double@8，speed u16@16。经纬度为原始 NMEA 度分数值，不能直接当十进制度；南西方向并未在记录中单独保存。double/float 为旧ARM数据表示，新增codec应明确验证小端IEEE754。

status偏移（页内）：timestamp u32@236，max_speed u16@240，average_speed u16@242，kcal float@244，average_humi u16@248，average_temp i16@250，total_distance u16@252，reserved@254，write_flag@255=0xA5。定点字段乘100四舍五入，解码除100；速度km/h、距离km。原写入无范围饱和保护。全零18B item为旧导出终止约定。

EEPROM：0 页索引1B；1 完成标志1B（A5完成/FF未完成）；2–3记录数u16（FFFF未初始化）；4–5磁校准kx；6–7 ky；8–15目的纬度double；16–23目的经度double。

测试分区未指定：STORAGE_WRITE_BLOCKED。没有读取或备份真实设备数据；包内未发现实机历史页样例。保留原读取器，禁止因扫描失败自动格式化。
