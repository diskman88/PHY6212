#include <uart.h>
#include <log.h>

#include <devices/uart.h>
#include "drv_usart.h"
extern dev_t *g_console_handle;

size_t StuffData(const uint8_t *ptr, size_t length, uint8_t *dst);
size_t UnStuffData(const uint8_t *ptr, size_t length, uint8_t *dst);


static uint8_t cobs_packet[1024];

static void voice_event_handle(voice_Evt_t *evt) 
{
    if (evt->type == HAL_VOICE_EVT_FAIL) {
        LOGI("VOICE", "voice event failed");
    }
    else if (evt->type == HAL_VOICE_EVT_DATA) {
        // LOGI("VOICE", "voice new data:size = %d, addr = %x", evt->size, (uint32_t)evt->data);
        // LOG_HEXDUMP("VOICE", evt->data, evt->size * 4);
        int len = StuffData((uint8_t *)evt->data, evt->size*4, cobs_packet);
        cobs_packet[len] = 0;
        uart_send(g_console_handle, cobs_packet, len + 1);
    } else {
        return;
    }
}

void audio_service_init()
{
    phy_voice_init();
    
    phy_gpio_cfg_analog_io(GPIO_P18, Bit_ENABLE);
    phy_gpio_cfg_analog_io(GPIO_P19, Bit_ENABLE);
    // phy_gpio_cfg_analog_io(GPIO_P20, Bit_ENABLE);

    voice_Cfg_t cfg;
    cfg.voiceSelAmicDmic = false;   // 模拟麦克风
    cfg.amicGain = 10;
    cfg.voiceGain = 40;
    cfg.voiceEncodeMode = VOICE_ENCODE_BYP;
    cfg.voiceRate = VOICE_RATE_16K;
    cfg.voiceAutoMuteOnOff = true;
    phy_voice_config(cfg, voice_event_handle);

    phy_voice_start();
}

void audio_start()
{
    phy_voice_start();
}

void audio_stop()
{
    phy_voice_stop();
}