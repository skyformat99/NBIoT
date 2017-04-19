/******************** (C) COPYRIGHT 2017 陆超 **********************************
* File Name          :  LCD.c
* Author             :  陆超
* CPU Type           :  nRF51802
* IDE                :  IAR 7.8
* Version            :  V1.0
* Date               :  04/09/2017
* Description        :  LCD应用程序
*******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "LCD.h"
#include "nrf_gpio.h"
#include "nRF51_BLE_Private_Service.h"
#include "OLED.h"
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
        
/* Private function prototypes -----------------------------------------------*/
void LCD_Task_Handle(void *p_arg);                                      // LCD任务
void LCD_Task_Create(void);                                             // 创建LCD任务
void LCD_Prepare_Screen_Sensor(void);									// 准备sensor页面
void LCD_Screen_Sensor(void);											// 显示sensor页面

u32 LCD_Chip_Init(void);                                                // 芯片初始化


/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
*                           陆超@2017-04-09
* Function Name  :  LCD_Task_Create
* Description    :  创建LCD任务
* Input          :  None
* Output         :  None
* Return         :  None
*******************************************************************************/
void LCD_Task_Create(void)
{
    u32 err_code = NRF_SUCCESS;

    // 初始化变量和IO
    LCD_Variable_Init();
    LCD_Port_Init();
    
    // 配置参数 周期模式运行
    LCD_Task.Run_Mode        = APP_TIMER_MODE_REPEATED;
    LCD_Task.Timeout_handler = LCD_Task_Handle;
    LCD_Task.Period          = TASK_LCD_PERIOD;

    err_code |= app_timer_create(&LCD_Task.p_ID,
                                 LCD_Task.Run_Mode,
                                 LCD_Task.Timeout_handler);
    // 芯片初始化
    err_code |= LCD_Chip_Init();

    if (err_code != NRF_SUCCESS)
    {
        app_trace_log("LCD芯片初始化失败!\r\n");    
    }
    else
    {
        err_code |= Task_Timer_Start(&LCD_Task, NULL);
        if (err_code != NRF_SUCCESS)
        {
            app_trace_log("Task LCD create failed!\r\n");    
        }
    }

	// 准备首页数据
	LCD_Prepare_Screen_Sensor();

}// End of void LCD_Task_Create(void)

/*******************************************************************************
*                           陆超@2017-04-09
* Function Name  :  LCD_Chip_Init
* Description    :  芯片初始化
* Input          :  None
* Output         :  None
* Return         :  NRF_SUCCESS 成功 1失败
*******************************************************************************/
u32 LCD_Chip_Init(void)
{
    u32 Err_Code = NRF_SUCCESS;

	OLED_Init();

    return Err_Code;
        
}// End of u32 LCD_Chip_Init(void)

/*******************************************************************************
*                           陆超@2017-04-09
* Function Name  :  LCD_Task_Handle
* Description    :  LCD任务
* Input          :  void *p_arg
* Output         :  None
* Return         :  None
*******************************************************************************/
void LCD_Task_Handle(void *p_arg)
{

	switch(LCD.Screen_Show)
	{
		case SCREEN_SENSOR:
		{
			LCD_Screen_Sensor();
			
		}break;

		case SCREEN_TIME:
		{

			
		}break;

		default:
		break;
		
	}

   
}// End of void LCD_Task_Handle(void *p_arg)

/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  LCD_Prepare_Screen_Sensor
* Description    :  准备sensor页面
* Input          :  None
* Output         :  None
* Return         :  None
*******************************************************************************/
void LCD_Prepare_Screen_Sensor(void)
{
	OLED_CLS();
	
    // 显示温湿度标号
	OLED_String_8x16(0,0, "Temp: ", sizeof("Temp: ") - 1);
	OLED_String_8x16(0,16, "Humi: ", sizeof("humi: ") - 1);
	OLED_String_16x16(96, 0, (u8*)Temp_Unit, 1);
	OLED_String_8x16(102, 16, "%", sizeof("%") - 1);

	// tVOC eCO2
	OLED_String_8x16(0,32, "tVOC:  ...", sizeof("tVOC:  ...") - 1);
	OLED_String_8x16(0,48, "eCO2:  ...", sizeof("eCO2:  ...") - 1);
	OLED_String_8x16(96, 32, "ppb", sizeof("ppb") - 1);
	OLED_String_8x16(96, 48, "ppm", sizeof("ppm") - 1);

	LCD.Screen_Now = SCREEN_SENSOR;
	
}// End of void LCD_Prepare_Screen_Sensor(void)

/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  LCD_Screen_Sensor
* Description    :  显示sensor页面
* Input          :  None
* Output         :  None
* Return         :  None
*******************************************************************************/
void LCD_Screen_Sensor(void)
{
	u8 ucTemp[6];
	u8 ucLen;

	// 判断当前是否sensor页面
	if (LCD.Screen_Now != SCREEN_SENSOR)
	{
		LCD_Prepare_Screen_Sensor();
	}
	
	// 温度大于0
	if (Sensor.sTemp >= 0)
	{
		// 十度内
		if (Sensor.sTemp < 100)
		{
			sprintf((char *)ucTemp, "  %01d.%01d", Sensor.sTemp / 10, Sensor.sTemp % 10);
		}
		else
		{
			sprintf((char *)ucTemp, " %02d.%01d", Sensor.sTemp / 10, Sensor.sTemp % 10);
		}
		
	}
	else
	{
		Sensor.sTemp = abs(Sensor.sTemp);
		sprintf((char *)ucTemp, "-%02d.%01d", Sensor.sTemp / 10, Sensor.sTemp % 10);

	}
	OLED_String_8x16(6 * 8, 0, ucTemp, 5);
	
	// 湿度
	sprintf((char *)ucTemp, " %02d.%01d", Sensor.usHumi / 10, Sensor.usHumi % 10);
	OLED_String_8x16(6 * 8, 16, ucTemp, 5);
	
	// tVOC eCO2
	ucLen = sprintf((char *)ucTemp, " %4d", Sensor.tVOC);
	OLED_String_8x16(6 * 8, 32, ucTemp, ucLen);
	ucLen = sprintf((char *)ucTemp, " %4d", Sensor.eCO2);
	OLED_String_8x16(6 * 8, 48, ucTemp, ucLen);

	
}// End of void LCD_Screen_Sensor(void)


/******************* (C) COPYRIGHT 2017 陆超 ************* END OF FILE ********/




