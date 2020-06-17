
#include "audio_service.h"
#include <uart.h>
#include <log.h>

static void voice_event_handle(voice_Evt_t *evt) 
{
    if (evt->type == HAL_VOICE_EVT_FAIL) {
        LOGI("VOICE", "voice event failed");
    }
    else if (evt->type == HAL_VOICE_EVT_DATA) {
        // LOGI("VOICE", "voice new data:size = %d, addr = %x", evt->size, (uint32_t)evt->data);
        LOG_HEXDUMP("VOICE", evt->data, evt->size * 4);
    } else {
        return;
    }
}

void audio_service_init()
{
    phy_voice_init();
    
    voice_Cfg_t cfg;
    cfg.voiceSelAmicDmic = false;   // 模拟麦克风
    cfg.amicGain = 0;
    cfg.voiceGain = 40;
    cfg.voiceEncodeMode = VOICE_ENCODE_BYP;
    cfg.voiceRate = VOICE_RATE_8K;
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



