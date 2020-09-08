/**
 * 
 * 
 * 
 * 
 */

#include "battery.h"
#include "aos/log.h"


int read_bat_voltage(uint32_t pin, adc_CH_t adc_ch)
{
    adc_conf_t sconfig;
    adc_handle_t hd;
    uint32_t data[16];
    int ret = 0;
    //1.引脚配置为模拟输入
    drv_pinmux_config(pin, ADCC);
    //2.初始化ADC设备
    hd = drv_adc_initialize(0, NULL);
    if (!hd) {
        LOGE("BATTERY", "adc init faield");
        return -1;
    }
    //3.配置ADC工作模式
    sconfig.mode = ADC_CONTINUOUS;
    sconfig.trigger = 0;
    sconfig.intrp_mode = 0;
    sconfig.channel_array = (uint32_t []){adc_ch};
    sconfig.channel_nbr = 1;
    sconfig.conv_cnt = 16;
    sconfig.enable_link_internal_voltage = 1;   // 该输入通道连接到VIN(Vbat)
    sconfig.link_internal_voltage_channel = adc_ch;   
    ret = drv_adc_config(hd, &sconfig);
    if (ret != 0) {
        LOGE("BATTERY", "adc config failed:0x%X", ret);
        return -1;
    }
    //4.启动ADC测量
    if (drv_adc_start(hd) != 0) {
        LOGE("BATTERY", "adc start failed");
        return -1;
    }
    //5.读取采样结果
    ret = drv_adc_read(hd, &data[0], 16);
    //6.停止ADC
    if (drv_adc_stop(hd) != 0) {
        LOGE("BATTERY", "adc stop failed");
        return -1;
    }
    //7.释放ADC
    if (drv_adc_uninitialize(hd) != 0) {
        LOGE("BATTERY", "adc uninitial failed");
        return -1;
    }
    //8.返回平均值
    uint32_t sum;
    sum = 0; 
    for (int i = 0; i < 16; i++) {
        sum += data[i];
    }        
    sum = sum / 16;
    return sum;
}

int battery_get_level()
{
    int vol = read_bat_voltage(BATTERY_GPIO, BATTERY_ADC_CH);
    if (vol == -1) {
        LOGE("BATTERY", "battery read failed");
        return 0;
    }

    if (vol < 2500) {
        return 5;
    }
    else if (vol < 2700) {
         return 10;
    }
    else if (vol < 2800) {
        return 25;
    }
    else if (vol < 2900) {
        return 50;
    }
    else if (vol < 3000) {
        return 75;
    }
    else {
        return 100;
    }    
}