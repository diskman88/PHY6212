/******************************************************************************
 * @file     IR.c
 * @brief    用PWM驱动红外发射
 * @version  V1.0
 * @date     2020.6.3-2020.6.11
 ******************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dw_gpio.h"
#include "pin_name.h"
#include "pinmux.h"
#include <pm.h>
#include <ARMCM0.h>

#include <devices/devicelist.h>
#include <devices/device.h>
#include <devices/driver.h>
#include "drv_gpio.h"//GPIO头文件
#include "pwm.h"//pwm头文件,修改版

#include "IR.h"

/*********** 参数定义 ***********/
#define NEC_H       (420)
#define NEC_0L      (520)
#define BitInByte   (8)
#define PWM_IDX     (0)


/*********** 全局变量 ***********/
pwm_handle_t IR_PWM_HANDLE = NULL;//PWM句柄
pin_name_e IR_GPIO = GPIO_P06;//默认使用GPIO_P06作为IR
pin_func_e IR_PWM = PWM1;//默认使用PWM1生成载波
uint8_t PWM_CH;//pwm通道号



/**
 * \param FreOut :输出频率值
 * \param BisectionNum  :将一段PWM做N等分
 * \param HLevel  :高电平占几分
 * 
 * #define PWM_BASE_FREQ 16000000
 **/
/*
int32_t csi_pwm_config(pwm_handle_t handle,uint8_t channel,uint32_t FreOut,uint32_t BisectionNum,uint32_t HLevel)
{
    PWM_NULL_PARAM_CHK(handle);

    if (BisectionNum < 1 || HLevel < 1) { //占空比比例小于1或者高电平比例小于1
        return ERR_PWM(DRV_ERROR_PARAMETER);
    }

    pwm_Ctx_t *base = handle;
    ck_pwm_priv_t *p = &base->ch[channel];
    int ch_en = base->ch_en[p->pwmN];

    if (ch_en == TRUE) {
        csi_pwm_stop(handle, channel);
    }

    base->ch_en[channel] = false;
    p->pwmN = channel;
    p->pwmPolarity = PWM_POLARITY_FALLING;
    p->pwmMode = PWM_CNT_UP;

    uint32_t div = BIT(p->pwmDiv & 0XFF);

    p->cntTopVal = 16000000 / div / FreOut;
    p->cmpVal = p->cntTopVal * HLevel / BisectionNum;

    if (p->cmpVal <= 0) {
        p->cmpVal = 1;
    }

    phy_pwm_init(p->pwmN, p->pwmDiv, p->pwmMode, p->pwmPolarity);
    phy_pwm_set_count_val(channel, p->cmpVal, p->cntTopVal);

    if (ch_en == TRUE) {
        csi_pwm_start(handle, channel);
    }

    return 0;
}
*/

/**
  \brief        us级延时函数
  \param[time]  time取值在1~1000之间,即1us~1ms
*/
static void DelayMicrosecond(uint32_t time)
{
    uint32_t load  = SysTick->LOAD;//47999
    uint32_t start = SysTick->VAL;//起始值
    uint32_t cnt   = 47 * time;

    if(time>1000)time=1000;
    else if(time<1)time=1;

    while(1)
    {
        uint32_t cur = SysTick->VAL;//当前值

        if(start > cur){
            if (start - cur >= cnt){
                return;
            }
        }else {
            if (load - cur + start >= cnt){
                return;
            }
        }
    }    
}

/**
  \brief        发射Leader1
*/
void SendLead_1()
{
    int i=0;
    /************************************************************************
    载波    #define NEC_LH  (9*1000)
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, IR_PWM);//配置引脚为PWM功能

    csi_pwm_start(IR_PWM_HANDLE, PWM_CH);//启动PWM
    for(i=0;i<9;i++)DelayMicrosecond(1000);
    /************************************************************************
    空波    #define NEC_LL  (4.5*1000)
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, PIN_FUNC_GPIO);
    phy_gpio_write(IR_GPIO, 0);
    for(i=0;i<4;i++)DelayMicrosecond(1000);
    DelayMicrosecond(500);
}

/**
  \brief            发射8位字节数据
  \param[bit8byte]  字节数据内容
*/
void Send8BitByte(uint32_t bit8byte)
{
    int whichBit=0,checkbit=0;

    for(whichBit=0;whichBit<BitInByte;whichBit++)
    {
        checkbit = bit8byte & (0x01 << whichBit);
        /************************************************************************
        载波    #define NEC_H   (560)
        ************************************************************************/
        drv_pinmux_config(IR_GPIO, IR_PWM);//配置引脚为PWM功能
        csi_pwm_start(IR_PWM_HANDLE, PWM_CH);//启动PWM
        DelayMicrosecond(NEC_H);
        /************************************************************************
        空波    #define NEC_0L  (560)   #define NEC_1L  (1350)
        ************************************************************************/
        drv_pinmux_config(IR_GPIO, PIN_FUNC_GPIO);
        phy_gpio_write(IR_GPIO, 0);
        if(checkbit)
        {
            DelayMicrosecond(1000);
            DelayMicrosecond(690);
            // printf("1");
        }else if(checkbit == 0){
            DelayMicrosecond(NEC_0L);
            // printf("0");
        }        
    }
}

/**
  \brief        发射停止位
*/
void SendStopBit()
{
    int i=0;
    /************************************************************************
    载波    #define NEC_H   (560)
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, IR_PWM);//配置引脚为PWM功能
    csi_pwm_start(IR_PWM_HANDLE, PWM_CH);//启动PWM
    DelayMicrosecond(NEC_H);
    /************************************************************************
    空波 需要计算剩余时长
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, PIN_FUNC_GPIO);
    phy_gpio_write(IR_GPIO, 0);
    for(i=0;i<40;i++)DelayMicrosecond(1000);
}

/**
  \brief        发射Leader2
*/
void SendLead_2()
{
    int i;
    /************************************************************************
    载波    #define NEC_LH  (9*1000)
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, IR_PWM);//配置引脚为PWM功能
    csi_pwm_start(IR_PWM_HANDLE, PWM_CH);//启动PWM
    for(i=0;i<9;i++)DelayMicrosecond(1000);

    /************************************************************************
    空波    #define NEC_2LL (2.25*1000)
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, PIN_FUNC_GPIO);
    phy_gpio_write(IR_GPIO, 0);
    for(i=0;i<10;i++)DelayMicrosecond(225);

    /************************************************************************
    载波    #define NEC_H   (560)
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, IR_PWM);//配置引脚为PWM功能
    csi_pwm_start(IR_PWM_HANDLE, PWM_CH);//启动PWM
    DelayMicrosecond(NEC_H);

    /************************************************************************
    空波
    ************************************************************************/
    drv_pinmux_config(IR_GPIO, PIN_FUNC_GPIO);
    phy_gpio_write(IR_GPIO, 0);
    for(i=0;i<96;i++)DelayMicrosecond(1000);
    DelayMicrosecond(190);
}

/**
  \brief            准备IR
*/
void IR_Ready()
{
    IR_PWM_HANDLE = csi_pwm_initialize(PWM_IDX);//初始化PWM句柄
}

/**
  \brief            注销IR
*/
void IR_Close()
{
    csi_pwm_uninitialize(IR_PWM_HANDLE);//注销PWM
}


/**
  \brief                    配置IR PWM参数
  \param[PWM_CH]            pwm通道号
  \param[FreOut]            输出频率值
  \param[BisectionNum]      将一段PWM做N等分
  \param[HLevel]            高电平占几分
*/
int IR_Config_PWM(uint8_t PwmCH,uint32_t FreOut,uint32_t BisectionNum,uint32_t HLevel)
{
    PWM_CH = PwmCH;
    csi_pwm_config( /*句柄*/IR_PWM_HANDLE, /*通道号*/PWM_CH,/*输出频率*/FreOut,/*N等分*/BisectionNum,/*高电平占比*/HLevel);
    
    return 0;
}

/**
  \brief                 配置IR GPIO参数
  \param[pin]            IR引脚号
  \param[pin_func]       IR引脚开启的PWM编号
*/
int IR_Config_GPIO(pin_name_e pin, pin_func_e pin_func)
{
    IR_GPIO = pin;
    IR_PWM = pin_func;
    
    return 0;
}

void IR_Send(uint32_t UserCode,uint32_t KeyCode)
{
    uint32_t _UserCode = ~UserCode;
    uint32_t _KeyCode = ~KeyCode;

    printf("IR Send\n");

    /* LeadCode */
    SendLead_1();
    
    /* 用户码 */
    Send8BitByte(UserCode);//用户码
    Send8BitByte(_UserCode);//用户码反码

    /* 键值 */
    Send8BitByte(KeyCode);//键值
    Send8BitByte(_KeyCode);//键值反码

    /* 停止码 */
    SendStopBit();

    /* LeadCode */
    SendLead_2();

    printf("IR had send\n");
}