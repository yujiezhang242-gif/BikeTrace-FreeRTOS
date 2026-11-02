# BikeTrace-FreeRTOS
这个项目是一个基于 STM32 + FreeRTOS 的智能骑行导航终端，系统采用分层+多任务架构设计：底层通过 UART、I2C、SPI 接入 4G、GPS、温湿度、电子罗盘、EEPROM 和外部 Flash 等外设，中间层封装设备驱动，上层通过 FreeRTOS 将功能拆分为骑行主任务、云端通信任务、存储任务和 UI 任务，通过队列和信号量实现协同。
