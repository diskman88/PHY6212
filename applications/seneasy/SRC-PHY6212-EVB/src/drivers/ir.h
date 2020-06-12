#ifndef __IR_H_
#define __IR_H_


/**
  \brief        发射Leader1
*/
void SendLead_1();

/**
  \brief            发射8位字节数据
  \param[bit8byte]  字节数据内容
*/
void Send8BitByte(uint32_t bit8byte);

/**
  \brief        发射停止位
*/
void SendStopBit();

/**
  \brief        发射Leader2
*/
void SendLead_2();

/**
  \brief            准备IR
*/
void IR_Ready();

/**
  \brief                    配置IR PWM参数
  \param[PWM_CH]            pwm通道号
  \param[FreOut]            输出频率值
  \param[BisectionNum]      将一段PWM做N等分
  \param[HLevel]            高电平占几分
*/
int IR_Config_PWM(uint8_t PwmCH,uint32_t FreOut,uint32_t BisectionNum,uint32_t HLevel);

/**
  \brief                 配置IR GPIO参数
  \param[pin]            IR引脚号
  \param[pin_func]       IR引脚开启的PWM编号
*/
int IR_Config_GPIO(pin_name_e pin, pin_func_e pin_func);

/**
  \brief            注销IR
*/
void IR_Close();


void IR_Send(uint32_t UserCode,uint32_t KeyCode);

#endif