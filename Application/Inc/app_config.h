#ifndef APP_CONFIG_H
#define APP_CONFIG_H
/* C8 LITE: three tasks. P1 target overrides APP_HEARTBEAT_ONLY. */
#ifndef APP_HEARTBEAT_ONLY
#define APP_HEARTBEAT_ONLY 0
#endif
#define APP_CONTROL_STACK_WORDS 384U
#define APP_SERVICE_STACK_WORDS 768U
#define APP_UI_STACK_WORDS 384U
#define APP_GPS_STREAM_SIZE 256U
#define APP_NET_STREAM_SIZE 512U
#define APP_STORAGE_QUEUE_DEPTH 5U /* one slot always reserved for STOP */
#ifndef APP_NETWORK_ENABLED
#define APP_NETWORK_ENABLED 0
#endif
#ifndef APP_SERVER_HOST
#define APP_SERVER_HOST ""
#endif
#ifndef APP_SERVER_PORT
#define APP_SERVER_PORT 0U
#endif
/* No firmware build may silently enable legacy history writes. */
#ifndef APP_STORAGE_WRITE_ENABLED
#define APP_STORAGE_WRITE_ENABLED 0
#endif
#ifndef APP_FLASH_REGION_APPROVED
#define APP_FLASH_REGION_APPROVED 0
#endif
#ifndef APP_FLASH_REGION_START
#define APP_FLASH_REGION_START 0U
#endif
#ifndef APP_FLASH_REGION_END
#define APP_FLASH_REGION_END 0U
#endif
#define APP_OLED_ADDRESS 0x3CU
#define APP_AHT_ADDRESS 0x38U
#ifndef APP_MAGNETOMETER_HMC_CONFIRMED
#define APP_MAGNETOMETER_HMC_CONFIRMED 0
#endif
#endif
