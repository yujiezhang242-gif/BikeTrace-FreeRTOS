/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file flash_index.c
 * 
 * @par dependencies 
 * - flash_index.h
 * 
 * @author 
 * 
 * @brief Functions related to reading and writing in the chip's flash area.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-09-13
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
 
 /* *********************************** 存储优化说明 ********************************** 
 * 1.
 *
 *
 *
 *
 *
 ************************************************************************************* */
 
/* Includes ------------------------------------------------------------------*/
#include "flash_index.h"
#include "w25qxx.h"

/* Private define ------------------------------------------------------------*/
#define SECTOR_SIZE                           (4096)        		// 0x1000
// 索引数据部分
#define INDEX_SECTOR0_START                   (0x0000)
#define INDEX_SECTOR1_START (INDEX_SECTOR0_START + 1 * SECTOR_SIZE) // 0x1000

// 数据项部分
#define TABLE0_START  (INDEX_SECTOR1_START + 1 * SECTOR_SIZE)    	// 0x2000
#define TABLE1_START  (TABLE0_START  + 31 * SECTOR_SIZE)      	    // 0x21000
#define TABLE2_START  (TABLE1_START  + 31 * SECTOR_SIZE)      	    // 0x40000
#define TABLE3_START  (TABLE2_START  + 32 * SECTOR_SIZE)      	    // 0x60000
#define TABLE4_START  (TABLE3_START  + 32 * SECTOR_SIZE)      	    // 0x80000
#define TABLE5_START  (TABLE4_START  + 32 * SECTOR_SIZE)      	    // 0xA0000
#define TABLE6_START  (TABLE5_START  + 32 * SECTOR_SIZE)      	    // 0xC0000
#define TABLE7_START  (TABLE6_START  + 32 * SECTOR_SIZE)      	    // 0xE0000
#define TABLE8_START  (TABLE7_START  + 32 * SECTOR_SIZE)      	    // 0x100000
#define TABLE9_START  (TABLE8_START  + 32 * SECTOR_SIZE)      	    // 0x120000
#define TABLE10_START (TABLE9_START  + 32 * SECTOR_SIZE)     	    // 0x140000
#define TABLE11_START (TABLE10_START + 32 * SECTOR_SIZE)    	    // 0x160000
#define TABLE12_START (TABLE11_START + 32 * SECTOR_SIZE)    	    // 0x180000
#define TABLE13_START (TABLE12_START + 32 * SECTOR_SIZE)    	    // 0x1A0000
#define TABLE14_START (TABLE13_START + 32 * SECTOR_SIZE)    	    // 0x1C0000
#define TABLE15_START (TABLE14_START + 32 * SECTOR_SIZE)    	    // 0x1E0000

// 结束地址，暂未使用
#define FLASH_END (TABLE15_START + 32 * SECTOR_SIZE)            // 0x200000     2MB大小

#define W25QXX_PAGE_SIZE 256            /**< flash页大小 */
#define TABLE_CNT 16                    /**< index区域保存的表数目 */

/**< [Pro新增] */
#define PAGE_SIZE 16                    /**< index区域保存的页数目 */

/* Private typedef -----------------------------------------------------------*/

const static uint32_t table_addr[TABLE_CNT] = {
    TABLE0_START ,
    TABLE1_START ,
    TABLE2_START ,
    TABLE3_START ,
    TABLE4_START ,
    TABLE5_START ,
    TABLE6_START ,
    TABLE7_START ,
    TABLE8_START ,
    TABLE9_START ,
    TABLE10_START,
    TABLE11_START,
    TABLE12_START,
    TABLE13_START,
    TABLE14_START,
    TABLE15_START
};

struct flash_index {                        /**< flash索引结构体，76字节 */
    uint16_t ntables;                       /**< 已写入表的数量，2字节，范围 0~16 */
    uint16_t next_table_i;                  /**< 即将写入的表的下标 2字节，范围 0~15 */
    uint32_t table_ts[TABLE_CNT];           /**< 表的产生时间，也就是骑行开始时间，共16个，每个4字节 */
    uint32_t next_version;                  /**< 即将写入的版本号,4字节，每次累加1 */
	//uint32_t ride_times;					/**< [Pro新增] 即将写入的页索引，范围0~15 */
    //uint8_t  reserved[2];                   /**< 保留2字节 */
	uint16_t  next_page_i;					/**< [Pro新增] 即将写入的页索引，范围0~15 */
    uint8_t  next_no;                       /**< 即将写入的索引数据编号，范围 0~1 */
    uint8_t  write_flag;                    /**< 已写入标记位 0xA5 */
};


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static struct flash_index index;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

 /**********************************************************************************
  * 函数功能: 初始化Index结构体变量
  * 输入参数: 无
  */
static void initialize_index(void)
{
    memset(&index, 0, sizeof(struct flash_index));
    index.write_flag = FLASH_WRITE_FLAG;
    index.next_no = 0;
}

 /**********************************************************************************
  * 函数功能: 擦除指定地址扇区
  * 输入参数: 扇区地址
  */
void erase_index(uint32_t sector_addr)
{
    BSP_W25Qx_Erase_Block(sector_addr);
}

 /**********************************************************************************
  * 函数功能: 打印索引数据
  * 输入参数: 索引名称 / 索引结构体变量
  */
static void print_index(const char* desc, struct flash_index* index)
{
    int i;
    printf("======================= [%s] =========================\r\n", desc);
    
    printf("Index No: %u\r\n", index->next_no);
    printf("Version: %u\r\n", index->next_version);
	/* [优化] - 删除原有保留2字节内容 */
    //printf("Reserved: 0x%02X 0x%02X\r\n", index->reserved[0], index->reserved[1]); 
	/* [优化] - 替换为即将写入的页索引 */
	printf("[New]Page Index: %u\r\n", index->next_page_i);
    printf("Write Flag: 0x%02X\r\n", index->write_flag);
    printf("Total Tables: %u\r\n", index->ntables);
    printf("Table Index: %u\r\n", index->next_table_i);

    for (i = 0; i < TABLE_CNT; i++) {
        printf("Table Timestamp[%d]: %u\r\n", i, index->table_ts[i]);
    }
}

 /**********************************************************************************
  * 函数功能: 设置下一次写入索引信息
  * 输入参数: 无
  */
static void set_next_index(void)
{
	uint32_t sector_addr;

    /* 更新索引，设置下一次写入的信息 */
    index.next_version += 1;
	//index.ride_times += 1;
    index.next_table_i = (index.next_table_i + 1) % TABLE_CNT; // 循环更新
	
	/**< [Pro新增] */
	index.next_page_i += 1;
	if (index.next_page_i == PAGE_SIZE)
	{
		/* 循环更新 */
		index.next_page_i = 0;
		/* 切换扇区 */
		index.next_no = (index.next_no == 0) ? 1 : 0;
		
		sector_addr = (index.next_no == 0) ? INDEX_SECTOR0_START : INDEX_SECTOR1_START;
		/* 切换扇区前擦除 */
		erase_index(sector_addr);
		printf("erase_index\r\n");
	}
    
    print_index("next index", &index);
}

 /**********************************************************************************
  * 函数功能: 寻找有效的索引数据
  * 输入参数: 索引所在扇区编号
  * 返回参数：返回有效索引数据的页索引（范围0~15）
  */
static uint16_t serch_valid_page_index(uint32_t sector_addr)
{
    uint16_t valid_page_index = 0, i;
//	char desc[7];
	
	struct flash_index newindex;
	
	/* 遍历找到有效索引 */
	for (i = 0; i < PAGESIZE; i++)
	{
		memset(&newindex, 0, sizeof(struct flash_index));
		BSP_W25Qx_Read((uint8_t*)&newindex, sector_addr + i * W25QXX_PAGE_SIZE, sizeof(struct flash_index));
		
		if (newindex.write_flag == FLASH_WRITE_FLAG)
		{
			/* 优化：选择扇区内版本号最高的有效数据索引 */
			valid_page_index = i;
			/* 调试信息 */
//			sprintf(desc, "page %d", i);
//			print_index(desc, &newindex);
		}
		else
		{
			break;
		}
	}
	
	return valid_page_index;
}

 /**********************************************************************************
  * 函数功能: 往Flash写入索引信息
  * 输入参数: 页地址
  */
static void write_index_to_flash(uint32_t page_addr)
{
    uint8_t buffer[W25QXX_PAGE_SIZE] = {0};

    printf("write page to 0x%x\r\n", page_addr);

    memcpy(buffer, &index, sizeof(struct flash_index));
    BSP_W25Qx_Write(buffer, page_addr, W25QXX_PAGE_SIZE);
}

 /**********************************************************************************
  * 函数功能: 从Flash读取索引信息（获取下一次写入的索引扇区）
  * 输入参数: 0 - 异常  1 - 正常
  */
static uint8_t read_index_from_flash(void)
{
    struct flash_index index0, index1;
    struct flash_index *new;
    memset(&index0, 0, sizeof(struct flash_index));
    memset(&index1, 0, sizeof(struct flash_index));
	
	uint16_t index0_valid_page, index1_valid_page;
	
	/* [优化] - 搜索两个扇区最新数据所在页 */
	index0_valid_page = serch_valid_page_index(INDEX_SECTOR0_START);
	index1_valid_page = serch_valid_page_index(INDEX_SECTOR1_START);
	
	/* 如果两个扇区最新有效数据都为0，那么则为出厂状态 */
	if (index0_valid_page == 0 && index1_valid_page == 0)
	{
//		printf("earse full chip ...\r\n");
//		BSP_W25Qx_Erase_Chip();
//		printf("earse full chip succ\r\n");
		initialize_index();
	}
	/* 扇区0作为记录扇区，扇区1等待被切换 */
	else if (index0_valid_page !=0 && index1_valid_page == 0)
	{
		/* 读取有效数据 */
		BSP_W25Qx_Read((uint8_t*)&index0, INDEX_SECTOR0_START + \
			index0_valid_page * W25QXX_PAGE_SIZE, sizeof(struct flash_index));
		memcpy(&index, &index0, sizeof(struct flash_index));
		
		/* **** add code **** */
		
		if(judge_last_ride_finished_normal() == IS_RESTORE)
			return 0;
		
		/* **** add code **** */
		
		set_next_index();
	}
	/* 扇区1作为记录扇区，扇区0等待被切换 */
	else if (index1_valid_page !=0 && index0_valid_page == 0)
	{
		/* 读取有效数据 */
		BSP_W25Qx_Read((uint8_t*)&index1, INDEX_SECTOR1_START + \
			index1_valid_page * W25QXX_PAGE_SIZE, sizeof(struct flash_index));
		memcpy(&index, &index1, sizeof(struct flash_index));
		
		/* **** add code **** */
		
		if(judge_last_ride_finished_normal() == IS_RESTORE)
			return 0;
		
		/* **** add code **** */
		
		set_next_index();
	}
	/* 两个扇区都有数据存在,选择版本号高的 */
	else
	{
		BSP_W25Qx_Read((uint8_t*)&index0, INDEX_SECTOR0_START + \
			index0_valid_page * W25QXX_PAGE_SIZE, sizeof(struct flash_index));
		BSP_W25Qx_Read((uint8_t*)&index1, INDEX_SECTOR1_START + \
			index1_valid_page * W25QXX_PAGE_SIZE, sizeof(struct flash_index));
		if (index0.next_version > index1.next_version)
		{
			new = &index0;
		}
		else
		{
			new = &index1;
		}
		
		memcpy(&index, new, sizeof(struct flash_index));
		
		/* **** add code **** */
		
		if(judge_last_ride_finished_normal() == IS_RESTORE)
			return 0;
		
		/* **** add code **** */
		
		set_next_index();
	}
	
	return 1;

/* =================================== 旧版本 ========================================= */	

//	BSP_W25Qx_Read((uint8_t*)&index0, INDEX_SECTOR0_START, sizeof(struct flash_index));
//	BSP_W25Qx_Read((uint8_t*)&index1, INDEX_SECTOR1_START, sizeof(struct flash_index));

//    print_index("index 0", &index0);
//    print_index("index 1", &index1);

//    // index 0 、index 1 均已被写入数据，找版本最新的
//    if(index0.write_flag == FLASH_WRITE_FLAG && index1.write_flag == FLASH_WRITE_FLAG) {
//        if (index0.next_version == 0xFFFFFFFF) { // index 0 的版本被擦除，但flag未擦除，极少发生的情况
//            new = &index1;
//        } else if (index1.next_version == 0xFFFFFFFF) { // index 1 的版本被擦除，但flag未擦除，极少发生的情况
//            new = &index0;
//        } else {
//            new = (index0.next_version > index1.next_version) ? &index0 : &index1;
//        }
//        memcpy(&index, new, sizeof(struct flash_index));
//        set_next_index();
//    } else if(index0.write_flag == FLASH_WRITE_FLAG) { // index 0 被写入了，index 1 未被写入或刚擦掉时断电了，使用 index 0
//        memcpy(&index, &index0, sizeof(struct flash_index));
//        set_next_index();
//        printf("use index 0\r\n");
//    }  else if(index1.write_flag == FLASH_WRITE_FLAG) { // index 1 被写入了，index 0 刚擦掉时断电了，使用 index 1
//        memcpy(&index, &index1, sizeof(struct flash_index));
//        set_next_index();
//        printf("use index 1\r\n");
//    } else { // index 0 、index 1 均未被写入，出厂状态，第1次骑行
//        initialize_index(); //注意，如果此时flash已经在Plus或青春版写入过 A5 标记，则应先手动擦除整个flash
//        printf("initialize index data\r\n");
//    }

/* =================================== 旧版本 ========================================= */	

}


 /**********************************************************************************
  * 函数功能: 写Flash索引
  * 输入参数: 无
  */
void write_index(void)
{
    uint32_t sector_addr;
	uint32_t page_addr;

    sector_addr = (index.next_no == 0) ? INDEX_SECTOR0_START : INDEX_SECTOR1_START;
    page_addr = sector_addr + index.next_page_i * W25QXX_PAGE_SIZE;
	
    if(index.ntables < TABLE_CNT)
        index.ntables += 1;
    
    //printf("erase index:0x%x\r\n", sector_addr);
    //erase_index(sector_addr);
	
    write_index_to_flash(page_addr);
	
	/* ***** add code ***** */
	write_page_index_to_at24((uint8_t *)&index.next_page_i);
	// 清除骑行结束标志位
	clear_write_flag();
	
    set_next_index();
}

/**
  * @brief  设置一张表的产生时间，即本次骑行的开始时间
  * @param  ts UNIX时间戳
  * @retval 无
  */
void set_table_created_timestamp(uint32_t ts)
{
	printf("create the [%d] table, timestamp:%u\r\n", index.next_table_i, ts);
    index.table_ts[index.next_table_i] = ts;
}

/**
  * @brief  获取第n个表创建的时间戳，即第n次骑行的开始时间
  * @param  n 表编号，1~16，对应索引下标0~15
  * @retval 表创建的时间戳
  */
uint32_t get_table_n_created_timestamp(int n)
{
	int idx;
    if(index.ntables == TABLE_CNT) { // 循环保存了
        idx = (index.next_table_i + n - 1) % TABLE_CNT;
    } else { // 还未循环保存
        idx = n - 1;
    }

    return index.table_ts[idx];
}

/**
  * @brief  获取第n个表的起始地址
  * @param  n 表编号，1~16，对应索引下标0~15
  * @retval 第n个表的地址
  */
uint32_t get_table_n_addr(int n)
{
	int idx;
    if(index.ntables == TABLE_CNT) { // 循环保存了，最多16组记录
        idx = (index.next_table_i + n - 1) % TABLE_CNT;
    } else { // 还未循环保存
        idx = n - 1;
    }
    printf("get table [%d] start addr, idx:%d, addr:0x%x\r\n",
        n, idx, table_addr[idx]);
    return table_addr[idx];
}

/**
  * @brief  获取准备写入的表的起始地址
  * @param  无
  * @retval 准备写入的表的起始地址
  */
uint32_t get_next_table_addr(void)
{
	return table_addr[index.next_table_i];
}
/**
  * @brief  获取表数量，即骑行次数
  * @param  无
  * @retval 有多少个表，范围0~16
  */
uint16_t get_table_cnt(void)
{
	return index.ntables;
}

/**
  * @brief  获取页索引
  * @param  无
  * @retval 范围0~16
  */
uint16_t get_next_page_index(void)
{
	return index.next_page_i;
}

/**
  * @brief  读取flash中的骑行次数
  * @param  无
  * @retval 无
  */
uint32_t get_ride_times(void)
{
	return index.next_version + 1;
	//return (index.ride_times + 1);
}

/**
  * @brief  执行一次设置下一个索引函数
  * @param  无
  * @retval 无
  */
void excute_set_next_index_fuc(void)
{
	set_next_index();
}

/**
  * @brief  读取flash中的索引数据，对索引结构体初始化
  * @param  无
  * @retval 无
  */
void init_index(void)
{
	if (read_index_from_flash() != 0)
		clear_ride_record_count();
}



