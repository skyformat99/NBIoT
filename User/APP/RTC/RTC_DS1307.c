/******************** (C) COPYRIGHT 2017 陆超 **********************************
* File Name          :  RTC_DS1307.c
* Author             :  陆超
* CPU Type           :  nRF51802
* IDE                :  IAR 7.8
* Version            :  V1.0
* Date               :  04/19/2017
* Description        :  RTC_DS1307应用程序
*******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "RTC_DS1307.h"
#include "Communal_IIC.h"
#include "sensirion_common.h"
#include "nrf_delay.h"
#include <time.h>

/* Private variables ---------------------------------------------------------*/
static const u8 DayOfMon[12] = {31, 28, 31, 30 , 31, 30, 31, 31, 30, 31, 30, 31};  
/* Private function prototypes -----------------------------------------------*/
u8 DS1307_Get_Week(u8 Year, u8 Month, u8 Day);							// 获取星期
u8 DS1307_Set_Date(Time_t time);										// 设置时间
u8 DS1307_Get_Data(Time_t *time);										// 获取时间
u8 DS1307_Start(void);													// 启动
u32 DS1307_Year_TO_Unix(Time_t *time);									// 年月日转unix
void DS1307_Unix_TO_Year(u32 uiSec, Time_t *time);						// unix转年月日

/* Private functions ---------------------------------------------------------*/

u8 DS1307_dec2bcd(u8 val)
{
	return val + 6 * (val / 10);
}

/*
 * transform bcd value to deciaml
 */
static u8 DS1307_bcd2dec(u8 val) 
{
	return val - 6 * (val >> 4);
}

/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  DS1307_Get_Week
* Description    :  获取星期
* Input          :  u8 Year		年
*                   u8 Month	月	
*                   u8 Day		日
* Output         :  None
* Return         :  星期 1~7
*******************************************************************************/
u8 DS1307_Get_Week(u8 Year, u8 Month, u8 Day) 
{
	int iWeek = 0;  
    unsigned int y = 0, c = 0, m = 0, d = 0;  
  
    if ( Month == 1 || Month == 2 )  
    {  
        c = ( Year - 1 ) / 100;  
        y = ( Year - 1 ) % 100;  
        m = Month + 12;  
        d = Day;  
    }  
    else  
    {  
        c = Year / 100;  
        y = Year % 100;  
        m = Month;  
        d = Day;  
    }  

    // 蔡勒公式 
    iWeek = y + y / 4 + c / 4 - 2 * c + 26 * ( m + 1 ) / 10 + d - 1;    

    // iWeek为负时取模 
    iWeek = iWeek >= 0 ? ( iWeek % 7 ) : ( iWeek % 7 + 7 ); 

    //星期日不作为一周的第一天  
    if ( iWeek == 0 )    
    {  
        iWeek = 7;  
    } 

	return (u8)iWeek;

}// End of u8 DS1307_Get_Week(u8 Year, u8 Month, u8 Day) 

/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  DS1307_Read_Register
* Description    :  读寄存器
* Input          :  u8  ucRead_Addr 要读取的地址
*                   u8* pBuffer     缓存指针
*                   u16 usRead_Len  读取长度
* Output         :  None
* Return         :  1成功 0失败
*******************************************************************************/
u8 DS1307_Read_Register(u8 ucRead_Addr, u8* pBuffer, u16 usRead_Len)
{
    u8 Transfer_Succeeded = 1;

    if(usRead_Len == 0)
    {
        return 0;
    }

    Transfer_Succeeded &= SW_I2C_Transfer(&Communal_IIC_2, DS1307_WRITE_ADDR, &ucRead_Addr, 1, SW_I2C_DONT_SEND_STOP);
    Transfer_Succeeded &= SW_I2C_Transfer(&Communal_IIC_2, DS1307_READ_ADDR, pBuffer, usRead_Len, SW_I2C_NEED_SEND_STOP);

    nrf_delay_us(100);

    return Transfer_Succeeded;
    
}// End of u8 DS1307_Read_Register(u8 ucRead_Addr, u8* pBuffer, u16 usRead_Len)

/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  DS1307_Write_Register
* Description    :  写芯片寄存器
* Input          :  u8 ucWrite_Addr    	要写入的地址
*                   u8* pBuffer         缓存指针
*                   u16 usWrite_Len     写入长度 
* Output         :  None
* Return         :  1成功 0失败
*******************************************************************************/
u8 DS1307_Write_Register(u8 ucWrite_Addr, u8* pBuffer, u16 usWrite_Len)
{
    unsigned char Transfer_Succeeded = 1;

    // 发送写命令
    Transfer_Succeeded &= SW_I2C_Transfer(&Communal_IIC_2, DS1307_WRITE_ADDR, &ucWrite_Addr, 1, SW_I2C_DONT_SEND_STOP);
    
    // 长度保护
    if (usWrite_Len == 0)
    {
        return 0;
    }

    // 发送数据
    while (usWrite_Len-- && Transfer_Succeeded)
    {
        Transfer_Succeeded &= SW_IIC_Write_Byte(&Communal_IIC_2, *pBuffer);
        pBuffer++;
    }


    // 发送停止位
    Transfer_Succeeded &= SW_I2C_Stop_Condition(&Communal_IIC_2);

    nrf_delay_us(100);
    
    return Transfer_Succeeded;
    
}// End of u8 DS1307_Write_Register(u8 ucWrite_Addr, u8* pBuffer, u16 usWrite_Len)

/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  DS1307_Set_Date
* Description    :  设置时间
* Input          :  Time_t time 待设置的时间
* Output         :  None
* Return         :  1成功 0失败
*******************************************************************************/
u8 DS1307_Set_Date(Time_t time) 
{
	u8 Data[7];
	u8 ucLen = 0;
	// check bounds
	if (time.Second > 59 ||
	    time.Minute > 59 ||
	    time.Hour > 23   ||
	    time.Day < 1    || time.Day > 31    ||
	    time.Month < 1  || time.Month > 12  ||
	    time.Year > 99)
		return 0;

	// get day of week
	time.Week = DS1307_Get_Week(time.Year, time.Month, time.Day);

	// 准备数据
	Data[ucLen++] = DS1307_dec2bcd(time.Second) & 0x7F;
	Data[ucLen++] = DS1307_dec2bcd(time.Minute);
	Data[ucLen++] = DS1307_dec2bcd(time.Hour);
	Data[ucLen++] = DS1307_dec2bcd(time.Week);
	Data[ucLen++] = DS1307_dec2bcd(time.Day);
	Data[ucLen++] = DS1307_dec2bcd(time.Month);
	Data[ucLen++] = DS1307_dec2bcd(time.Year);

	// 写日期
	if (DS1307_Write_Register(0x00, Data, ucLen))
	{
		return 1;
	}
	else
	{
		return 0;
	}
	
}// End of u8 DS1307_Set_Date(Time_t time) 

/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  DS1307_Get_Data
* Description    :  读取时间
* Input          :  Time_t *time 读取的时间
* Output         :  None
* Return         :  1成功 0失败
*******************************************************************************/
u8 DS1307_Get_Data(Time_t *time) 
{
	u8 Data[7];
	u8 ucLen = 0;

	// 读取时间
	if (DS1307_Read_Register(0x00, Data, 7))
	{
		time->Second = DS1307_bcd2dec(Data[ucLen++] & 0x7F);
		time->Minute = DS1307_bcd2dec(Data[ucLen++]);
		time->Hour   = DS1307_bcd2dec(Data[ucLen++]);
		time->Week   = DS1307_bcd2dec(Data[ucLen++]);
		time->Day    = DS1307_bcd2dec(Data[ucLen++]);
		time->Month  = DS1307_bcd2dec(Data[ucLen++]);
		time->Year   = DS1307_bcd2dec(Data[ucLen++]);

		return 1;
	}

	return 0;

}// End of u8 DS1307_Get_Data(Time_t *time) 

/*******************************************************************************
*                           陆超@2017-04-22
* Function Name  :  DS1307_Year_TO_Unix
* Description    :  RTC年月日转Unix
* Input          :  void
* Output         :  None
* Return         :  1970年开始的秒
*******************************************************************************/
u32 DS1307_Year_TO_Unix(Time_t *time)
{

	u16 rYear, i, Cyear=0;
	u8 rMon, rDay, rHour, rMin, rSec;
	u32 CountDay = 0;
	
	rSec  = time->Second;
	rMin  = time->Minute;;
	rHour = time->Hour;
	rDay  = time->Day;
	rMon  = time->Month;
	rYear = time->Year + 2000;

	// 闰年
	for(i = 1970; i < rYear; i++)
	{
		if(((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0))	
		{
			Cyear++;
		}
	}

	// 到今年的天书
	CountDay = Cyear * 366 + (rYear -1970 - Cyear) * 365;

	// 闰年2月
	for(i = 1; i < rMon; i++)
	{
		if((i == 2) && (((rYear % 4 == 0) && (rYear % 100 != 0)) || (rYear % 400 == 0)))
		{
			CountDay += 29;
		}
		else
		{
			CountDay += DayOfMon[i-1];
		}
	}
	CountDay += (rDay - 1);

	// 天数转秒数
	CountDay = CountDay * (60 * 60 * 24) + (unsigned long)rHour * (60 * 60) + (unsigned long)rMin * 60 + rSec;

	// 北京时间转化为世界时间
	CountDay = CountDay - 8 * 3600;
	return CountDay;

}// End of u32 DS1307_Year_TO_Unix(Time_t *time)

/*******************************************************************************
*                           陆超@2017-04-22
* Function Name  :  DS1307_Unix_TO_Year
* Description    :  Unix转年月日
* Input          :  u32 uiSec		unix 秒
*					Time_t *time	转换后年月日
* Output         :  None
* Return         :  None
*******************************************************************************/
void DS1307_Unix_TO_Year(u32 uiSec, Time_t *time)
{
	u16 i,j,usDay;
	u32 uiDay;

	// 转换为北京时间
	uiSec = uiSec + 8 * 3600;

	// 计算过去天数和剩余秒数
	uiDay = uiSec / (60 * 60 * 24);        
	uiSec = uiSec % (60 * 60 * 24);
	
	i = 1970;

	// 换算年数
	while(uiDay > 365)
	{
		// 闰年
		if(((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0))
		{
			uiDay -= 366;
		}
		else
		{
			uiDay -= 365;
		}
		i++;
	}
	if((uiDay == 365) && !(((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0)))
	{
		uiDay -= 365;
		i++;
	}

	// 年份
	time->Year = i - 2000;
	for(j = 0; j < 12; j++)  
	{
		if((j == 1) && (((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0)))
		{
			usDay = 29;
		}
		else
		{
			usDay = DayOfMon[j];
		}
		if(uiDay >= usDay)
		{
			uiDay -= usDay;
		}	
		else
		{
			break;
		}	
	}
	time->Month  = j + 1;
	time->Day    = uiDay + 1 ;
	time->Hour   = ((uiSec / 3600)) % 24;
	time->Minute = (uiSec % 3600) / 60;
	time->Second = (uiSec % 3600) % 60;
	time->Week   = DS1307_Get_Week(time->Year, time->Month, time->Day);
	
}// End of void GetDateTimeFromSecond(u32 uiSec, Time_t *time)


/*******************************************************************************
*                           陆超@2017-04-19
* Function Name  :  DS1307_Start
* Description    :  启动时钟
* Input          :  None
* Output         :  None
* Return         :  1成功 0失败
*******************************************************************************/
u8 DS1307_Start(void) 
{
	u8 Data;

	// 读取时间
	if (DS1307_Read_Register(0x00, &Data, 1))
	{
		// 最高位为1时时钟停止，需要清零启动
		if (Data & 0x80)
		{
			Data &= 0x7F;
			if (DS1307_Write_Register(0x00, &Data, 1))
			{
				return 1;	
			}
			
			return 0;
		}

		return 1;
	}

	return 0;

}// End of u8 DS1307_Start(void) 

/******************* (C) COPYRIGHT 2017 陆超 ************* END OF FILE ********/




