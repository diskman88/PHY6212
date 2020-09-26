/******************************************************************************
 * @file     ir.c
 * @brief    用PWM驱动红外发射
 * @version  V1.0
 * @date     2020.6.3-2020.6.12
 ******************************************************************************/
#include "ir_nec.h"

#include "dw_gpio.h"
#include "pinmux.h"
#include <pm.h>
#include "log.h"

#include "clock.h"
#include "drv_gpio.h"//GPIO头文件
// #include "drv_pwm.h"
#include "drv_timer.h"
#include <pwm.h>
// #include <timer.h>
#include <gpio.h>



/*********** 参数定义 ***********/

#define PWM0_CTL0    *(volatile uint32_t *)(PWM_CH_BASE + 0)     // PWM.Channel0
#define PWM0_CTL1    *(volatile uint32_t *)(PWM_CH_BASE + 4)     // PWM.Channel0

timer_handle_t ir_timer = NULL;

const ir_nec_code_params_t nec_code_param[IR_NEC_CODE_NUM] = {
    [IR_NEC_CODE_D0] = { .h = 560, .l = 1690 },
    [IR_NEC_CODE_D1] = { .h = 560, .l = 560 },
    [IR_NEC_CODE_LEAD] =  { .h = 9000, .l = 4500 },
    [IR_NEC_CODE_STOP] = { .h = 560, .l = 40000 },
    [IR_NEC_CODE_C1] = { .h = 9000, .l = 2250 },
    [IR_NEC_CODE_C2] = { .h = 560, .l = 96190 },
};

static struct
{
    bool is_high_period;
    bool is_pending_to_stop;
    bool is_busy;
    uint8_t codes[36];  // 1*Lead + 16*custom + 16*key + 1*stop + 2*repeat
    uint8_t index;
    uint16_t cycle_times;
}ir_nec;


static inline void _ir_carry_on()
{
    // 引脚配置为强上拉
    // *(volatile uint32_t *)(0x4000F000 + 0x08) &= ~(0x03 << 13);         // b[14:13]:clear pull control of pin 4 (3*(Padx) + 1)
    // *(volatile uint32_t *)(0x4000F000 + 0x08) |= (0x02 << 13);          // 00:floating; 01:weak pull up; 10:strong pull up; 11:pull down  
/*
    // GPIO6输出0
    *(volatile uint32_t *)(0x40008000 + 0x00) |= (1U << 4);            // GPIO6 输出0
    *(volatile uint32_t *)(0x40008000 + 0x04) |= (1U << 4);             // GPIO6 配置为输出
    // IR引脚切换到GPIO6
    *(volatile uint32_t *)(0x40003800 + 0x0C) &= ~(1U << 4);            // 引脚关闭多路映射功能(默认映射=GPIO)    
*/
    // IR引脚切换到PWM0
    // *(volatile uint32_t *)(0x40003800 + 0x0C) |= (1U << 4);             // 引脚使能多路映射功能
    // *(volatile uint32_t *)(0x40003800 + 0x1C) &= ~(0x3F << 0);         // clear pad4 full mux function select b[5:0]
    // *(volatile uint32_t *)(0x40003800 + 0x1C) |= (PWM0 << 0);          // pad4 select function = PWM0(10)
    // 使能PWM输出
    PWM0_CTL0 |= (1U << 0);
    PWM_ENABLE_ALL;   
}

static inline void _ir_carry_off()
{
    // // 引脚配置为下拉
    // *(volatile uint32_t *)(0x4000F000 + 0x08) &= ~(0x03 << 13);         // b[14:13]:clear pull control of pin 4 (3*(Padx) + 1)
    // *(volatile uint32_t *)(0x4000F000 + 0x08) |= (0x3 << 13);           // 00:floating; 01:weak pull up; 10:strong pull up; 11:pull down       
    // // GPIO6输出0
    // *(volatile uint32_t *)(0x40008000 + 0x00) &= ~(1U << 4);            // GPIO6 输出0
    // *(volatile uint32_t *)(0x40008000 + 0x04) |= (1U << 4);             // GPIO6 配置为输出
    // // IR引脚切换到GPIO6
    // *(volatile uint32_t *)(0x40003800 + 0x0C) &= ~(1U << 4);            // 引脚关闭多路映射功能(默认映射=GPIO)

    // 关闭PWM输出
    PWM_DISABLE_ALL;
    PWM0_CTL0 &= ~(1U << 0);

}

void ir_timer_reload(uint32_t us)
{
    csi_timer_set_timeout(ir_timer, us - 147);
    csi_timer_start(ir_timer);
}

void ir_timer_stop()
{
    if(ir_timer != NULL) {
        csi_timer_stop(ir_timer);
        csi_timer_uninitialize(ir_timer);
        ir_timer = NULL;
    }
}

static void ir_timer_callback(int32_t idx, timer_event_e event) 
{
    static uint8_t code;
    if (ir_nec.is_high_period == false) {
        code = ir_nec.codes[ir_nec.index];
        ir_timer_reload(nec_code_param[code].h);
        
        _ir_carry_on();
        ir_nec.is_high_period = true;
    } else {
        ir_timer_reload(nec_code_param[code].l);
        _ir_carry_off();
        ir_nec.is_high_period = false;
        ir_nec.index++;
        // 是否循环
        if (ir_nec.index >= 34){
            if (ir_nec.is_pending_to_stop) {
                ir_timer_stop();
                _ir_carry_off();
                ir_nec.is_pending_to_stop = false;
                ir_nec.is_busy = false;
            }
        }
        if (ir_nec.index >= 36) {
            ir_nec.index = 34;
            // 超时
            ir_nec.cycle_times++;
            if (ir_nec.cycle_times > 100) { // 108ms * 100 = 10.8s
                ir_timer_stop();
                _ir_carry_off();
                ir_nec.is_pending_to_stop = false;
                ir_nec.is_busy = false;
                LOGI("IR", "ir send terminated,because of timeout");
            }
        }
    }
}

int ir_nec_start_send(uint8_t custom, uint8_t key)
{
    if (ir_nec.is_busy) {
        LOGI("IR", "ir is busy");
        return -1;
    }
    /** 构建发码数据 **/
    // 1. lead code
    ir_nec.codes[0] = IR_NEC_CODE_LEAD;
    // 2. custom + key 
    // [b7:0]custom + [b15:b8]~custom + [b23:b16]key + [b31:b24]~key, LSB first
    uint32_t data;
    data = 0;
    data |= (~key) & 0x000000FF;
    data <<= 8;
    data |=  (key) & 0x000000FF;
    data <<= 8;
    data |= (~custom) & 0x000000FF;
    data <<= 8;
    data |= custom & 0x000000FF;
    for(int i = 0; i < 32; i++) {
        if (data & 0x00000001) {
            ir_nec.codes[1 + i] = IR_NEC_CODE_D1;
        } else {
            ir_nec.codes[1 + i] = IR_NEC_CODE_D0;
        }
        data >>= 1;
    }
    // 3. stop
    ir_nec.codes[33] = IR_NEC_CODE_STOP;
    // 4. repeat
    ir_nec.codes[34] = IR_NEC_CODE_C1;
    ir_nec.codes[35] = IR_NEC_CODE_C2;

    ir_nec.index = 0;
    ir_nec.is_high_period = false;
    ir_nec.is_pending_to_stop = false;
    ir_nec.cycle_times = 0;
    ir_nec.is_busy = true;

    /** 载波-PWM发生器 ***/
    // 1.配置PWM参数
    clk_gate_enable(MOD_PWM);
    PWM0_CTL0 &= ~(1U << 0);          // 禁止PWM
    PWM0_CTL0 = (PWM_CLK_NO_DIV << 12) | (PWM_CNT_UP << 8) | (PWM_POLARITY_RISING << 4);
    // 2.设置占空比和周期
    PWM0_CTL0 &= ~(1U << 16);
    PWM0_CTL1 = ((421-126) << 16) | (421U);    // 频率38K(周期=26.31uS)，占空比1/3
    PWM0_CTL0 |= (1U << 16);              // 参数更新使能
    // 3.引脚配置
    // P04引脚配置为下拉
    // *(volatile uint32_t *)(0x4000F000 + 0x08) &= ~(0x03 << 13);         // b[14:13]:clear pull control of pin 4 (3*(Padx) + 1)
    // *(volatile uint32_t *)(0x4000F000 + 0x08) |= (0x3 << 13);           // 00:floating; 01:weak pull up; 10:strong pull up; 11:pull down  
    // P04引脚映射到PWM0
    // *(volatile uint32_t *)(0x40003800 + 0x0C) |= (1U << 4);             // 引脚使能多路映射功能
    // *(volatile uint32_t *)(0x40003800 + 0x1C) &= ~(0x3F << 0);         // clear pad4 full mux function select b[5:0]
    // *(volatile uint32_t *)(0x40003800 + 0x1C) |= (PWM0 << 0);          // pad4 select function = PWM0(10)   

    // P34引脚配置为下拉
    // *(volatile uint32_t *)(0x4000F000 + 0x10) &= ~(0x03 << 19);         // b[20:19]:clear pull control of pin 4 (3*(Padx) + 1)
    // *(volatile uint32_t *)(0x4000F000 + 0x10) |= (0x3 << 19);           // 00:floating; 01:weak pull up; 10:strong pull up; 11:pull down  
    // P34引脚映射到PWM0
    *(volatile uint32_t *)(0x40003800 + 0x10) |= (1U << 2);             // 引脚使能多路映射功能
    *(volatile uint32_t *)(0x40003800 + 0x38) &= ~(0x3F << 16);         // clear pad6 full mux function select b[21:16]
    *(volatile uint32_t *)(0x40003800 + 0x38) |= (PWM0 << 16);          // pad6 select function = PWM0(10)   

    /*** 发码定时器 ***/
    ir_timer = csi_timer_initialize(0, ir_timer_callback);
    if (ir_timer == NULL) {
        LOGE("IR", "ir send timer no valid");
        return -1;
    }
    csi_timer_config(ir_timer, TIMER_MODE_RELOAD);

    /*** 发送第一个脉冲 ***/
    ir_timer_callback(0,TIMER_EVENT_TIMEOUT);
    // _ir_carry_on();

    return 0;
}

int ir_nec_stop_send()
{
    ir_nec.is_pending_to_stop = true;

    return 0;
}

void ir_test_carry_on()
{
    _ir_carry_on();
}

void ir_test_carry_off()
{
    _ir_carry_off();
}

bool ir_test_is_busy()
{
    return ir_nec.is_busy;
}