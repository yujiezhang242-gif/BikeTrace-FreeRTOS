/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_hmc5883l_driver.c
 * 
 * @par dependencies 
 * - bsp_hmc5883l_driver.h
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
/* Includes ------------------------------------------------------------------*/
#include "bsp_hmc5883l_driver.h"
#include "bsp_at24c02_driver.h"

#include "math.h"

#include "iic.h"
/* Private typedef -----------------------------------------------------------*/

typedef struct
{
	int16_t xmag;
	int16_t ymag;
	int16_t zmag;
	int16_t kx; //calibration
	int16_t ky; //calibration
} hmc5883l_param_t;

static hmc5883l_param_t g_hmc_param;

typedef enum{//采样求平均
    Sample_1 = 0x00,
    Sample_2 = 0x20,
    Sample_4 = 0x40,
    Sample_8 = 0x60
}Sample_Avr_TypeDef;

typedef enum{
    UltraSlow = 0x00,//0.75hz
    SuperSlow = 0x04,//1.5hz
    Slow      = 0x08,//3hz
    Mediun    = 0x10,//15hz
    Fast      = 0x14,//30hz
    SuperFast = 0x18,//75hz
    UltraFast = 0x1C//220hz
}Rate_TypeDef;

typedef enum{
    Normal    = 0x00,//正常模式
    Positive  = 0x01,
    Negative  = 0x02,
    Temp_Only = 0x03//只打开温度计 oepn temperature sensor only
}MS_TypeDef;

typedef enum{
    Ga_0_88 = 0x00,//用于0.88Ga的磁场范围，这意味着这个是增益最大的
    GA_1_3  = 0x20,
    Ga_1_9  = 0x40,
    Ga_2_5  = 0x60,
    Ga_4_0  = 0x80,
    Ga_4_7  = 0xA0,
    Ga_5_6  = 0xC0,
    Ga_8_1  = 0xE0//用于8.1Ga的磁场测量范围，增益最小的
}Gain_TypeDef;//增益

typedef enum{
    Continue = 0x00,//连续转换
    Single   = 0x01,//单次触发
    Idle     = 0x02//空闲模式
}Mode_TypeDef;

typedef enum{
    Temp_DISABLE = 0x00,//连续转换
    Temp_ENABLE  = 0x80
}Temp_TypeDef;

struct RegAInitTypedef {//寄存器A的结构体
    Temp_TypeDef Temp_EN;               //@ref to Temp_TypeDef 查阅此处枚举
    Sample_Avr_TypeDef Sample_ave;      //@ref to Sample_Avr_TypeDef 查阅此处枚举
    Rate_TypeDef Rate;                  //@ref to Rate_TypeDef 查阅此处枚举
    MS_TypeDef MS_Mode;                 //@ref to MS_TypeDef 查阅此处枚举
};

struct RegBInitTypedef {//寄存器B的结构体
    Gain_TypeDef Gain_Set;
};

struct HMCModeReg {//寄存器C的结构体
    Mode_TypeDef Mode_Set;
};
/* Private define ------------------------------------------------------------*/

#define HMC5883_Address              (0x1E<<1)   //当SD0接高电平时 while the SD0 connect to HIGH

#define RegisterA                    (0x00)
#define RegisterB                    (0x01)
#define ModeReg                      (0x02)
#define X_MSB                        (0x03)
#define X_LSB                        (0x04)
#define Z_MSB                        (0x05)
#define Z_LSB                        (0x06)
#define Y_MSB                        (0x07)
#define Y_LSB                        (0x08)
#define StatusReg                    (0x09)
#define Identifiction_regA           (0x0A)
#define Identifiction_regB           (0x0B)
#define Identifiction_regC           (0x0C)
#define Temp_MSB                     (0x31)
#define Temp_LSB                     (0x32)
      					
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
static void  hmc_init(void);
static float hmc_Get_Angle();//得到方位角
static void  hmc_calibration();//校准

bsp_hmc5883l_driver_t hmc5883l =
{
	.init = hmc_init,
	.getAngle = hmc_Get_Angle,
	.calibration = hmc_calibration
};

/* Private functions ---------------------------------------------------------*/
void HMC5983_REG_Init(struct RegAInitTypedef* RegA,struct RegBInitTypedef* RegB,struct HMCModeReg* Mode_reg){
	uint8_t input[2] = {0};
	
	input[0] = RegisterA;
	input[1] = RegA->MS_Mode | RegA->Rate | RegA->Sample_ave;
	iic.write(HMC5883_Address, input, 2);
	
	input[0] = RegisterB;
	input[1] = RegB->Gain_Set;
	iic.write(HMC5883_Address, input, 2);
	
	input[0] = ModeReg;
	input[1] = Mode_reg->Mode_Set;
	iic.write(HMC5883_Address, input, 2);
}

static void hmc_init(void)
{
	// 上电延迟
	HAL_Delay(10);
	
	struct RegAInitTypedef MYregA;//寄存器A：正常模式+快速+8次采样+打开温度计
	MYregA.MS_Mode = Normal;
	MYregA.Rate = Fast;
	MYregA.Sample_ave = Sample_8;
	MYregA.Temp_EN = Temp_ENABLE;//打开温度计可以，打开温度补偿，提升磁力计的精度
	
	struct RegBInitTypedef MYregB;//寄存器B:测2.5Ga磁场，一般增益
	MYregB.Gain_Set = Ga_8_1;

	struct HMCModeReg MYModereg;//模式选择:连续模式
	MYModereg.Mode_Set = Continue;

	HMC5983_REG_Init(&MYregA,&MYregB,&MYModereg);
	
	int16_t kx,ky;
	
	at24c02.readBytes((uint8_t *)&kx, HMC_CALIB_KX_ADDR, 2);
	at24c02.readBytes((uint8_t *)&ky, HMC_CALIB_KY_ADDR, 2);
	
	if((kx == -1 && ky == -1) || (kx == 0 && ky == 0))
	//if((kx == -1 && ky == -1))
	{
		hmc5883l.calibration();
	}
}

void HMC5983_Get_Data(void){
	uint8_t ldata,hdata;
	uint8_t wdata;
	
	wdata = X_MSB;
	iic.write(HMC5883_Address, &wdata, 1);
	iic.read(HMC5883_Address, &hdata, 1);
	wdata = X_LSB;
	iic.write(HMC5883_Address, &wdata, 1);
	iic.read(HMC5883_Address, &ldata, 1);

	g_hmc_param.xmag = (hdata << 8) | ldata;
	
	wdata = Y_MSB;
	iic.write(HMC5883_Address, &wdata, 1);
	iic.read(HMC5883_Address, &hdata, 1);
	wdata = Y_LSB;
	iic.write(HMC5883_Address, &wdata, 1);
	iic.read(HMC5883_Address, &ldata, 1);

	g_hmc_param.ymag = (hdata << 8) | ldata;
	
	wdata = Z_MSB;
	iic.write(HMC5883_Address, &wdata, 1);
	iic.read(HMC5883_Address, &hdata, 1);
	wdata = Z_LSB;
	iic.write(HMC5883_Address, &wdata, 1);
	iic.read(HMC5883_Address, &ldata, 1);

	g_hmc_param.zmag = (hdata << 8) | ldata;
}

//得到方位角
static float hmc_Get_Angle()
{
	float angle=0;
	HMC5983_Get_Data();
	
	float raw_angle = (float)(g_hmc_param.ymag - g_hmc_param.ky)/(float)(g_hmc_param.xmag - g_hmc_param.kx);
	if(g_hmc_param.xmag > 0 && g_hmc_param.ymag > 0)angle = atan(raw_angle)*180 / 3.141592f;
	else if(g_hmc_param.xmag > 0 && g_hmc_param.ymag < 0)angle = 360 + atan(raw_angle)*180 / 3.141592f;
	else if(g_hmc_param.xmag == 0 && g_hmc_param.ymag > 0)angle = 90;
	else if(g_hmc_param.xmag == 0 && g_hmc_param.ymag < 0)angle = 270;
	else if(g_hmc_param.xmag < 0 )angle = 180 + atan(raw_angle)*180/3.141592f;
	if(angle>360)angle=360;
	if(angle<0)angle=0;
	
	//printf("angle = [%f]\r\n", angle);
	
	// 当前设备x轴和前进方向相反，对返回的角度进行处理
	angle = (angle > 180) ? angle - 180 : angle + 180;

	return angle;//如果返回了999那么是错误数据
}

//校准
static void  hmc_calibration()
{
	uint16_t i =0;
	int16_t XGaMax=0;
	int16_t YGaMax=0;
	int16_t XGaMin=0;
	int16_t YGaMin=0;

	while (i!=200)//采集200个点
	{
		HMC5983_Get_Data();
		XGaMax = XGaMax < g_hmc_param.xmag? g_hmc_param.xmag:XGaMax;//找X的最大值
		XGaMin = XGaMin > g_hmc_param.xmag? g_hmc_param.xmag:XGaMin;//找X的最小值
		YGaMax = YGaMax < g_hmc_param.ymag? g_hmc_param.ymag:YGaMax;//找Y的最大值
		YGaMin = YGaMin > g_hmc_param.ymag? g_hmc_param.ymag:YGaMin;//找Y的最小值
		HAL_Delay(50);//50*200 10s后结束采样
		i++;
	}

	g_hmc_param.kx = (XGaMax + XGaMin)/2;
	g_hmc_param.ky = (YGaMax + YGaMin)/2;
	
	// 保存校准值
	at24c02.writeBytes((uint8_t *)&g_hmc_param.kx, HMC_CALIB_KX_ADDR, 2);
	HAL_Delay(5);
	at24c02.writeBytes((uint8_t *)&g_hmc_param.ky, HMC_CALIB_KY_ADDR, 2);
}

uint8_t HMC5983_Status(){
	uint8_t status = StatusReg;
	uint8_t temp;
	
	iic.write(HMC5883_Address, &status, 1);
	iic.read(HMC5883_Address, &temp, 1);

	/*2事件发生条件，注意：数据在3和1之间发生跳变很正常，因为满足条件2(1)
		1.all six bytes have been read and the next measurement 
		starts, 
		2. The mode register is written, 
		3. The measurement configuration (CRA) is written,
		4. Power is reset.
	*/
	return temp;//如果返回3说明12事件都发生
}