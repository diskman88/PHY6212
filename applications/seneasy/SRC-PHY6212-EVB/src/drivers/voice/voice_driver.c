#include "voice_driver.h"

#include <uart.h>
#include <log.h>
#include <devices/uart.h>
#include "drv_usart.h"
#include "voice.h"
#include "pm.h"

#include "../../app_msg.h"

bool is_voice_busy = false;

/**
 * @brief voice_fifo:语音帧缓冲FIFO队列
 * 
 */
// static pcm_fifo_def_t pcm_fifo;
static voice_fifo_def_t voice_fifo;

/**
 * @brief trans_seq_id: 语音启动时重置为0, 每采集一帧序列+1
 * 
 */
static uint16_t trans_seq_id;

/**
 * @brief voice_raw_pcm: 原始语音数据
 * 
 */
// static short voice_raw_pcm[256];
// static voice_trans_frame_t new_frame;

/**
 * @brief 语音数据采集中断(每采集128个样本)
 * 
 * @param evt 
 */

short test_pcm_data[256] = {0,1,2,3,4,5,6,7,8,9,10};

static void voice_pcm_truck_cb(voice_Evt_t *evt) 
{
    if (evt->type == HAL_VOICE_EVT_FAIL) {
        LOGI("VOICE", "voice event failed");
    }
    else if (evt->type == HAL_VOICE_EVT_DATA) {
#if 0
        io_msg_t msg;
        msg.type = IO_MSG_VOICE;
        msg.subtype = IO_MSG_VOICE_NEW_DATA;
        msg.lpMsgBuff = evt->data;      // 语音消息必须及时处理，否则会导致pcm数据缓冲区在中断里面被改写
        if (app_send_io_message(&msg) == false) {
            // LOGE("VOICE", "voice send message failed");
        }
        // phy_gpio_write(GPIO_P23, (trans_seq_id & 0x01));
        trans_seq_id++;
#else
        phy_gpio_write(GPIO_P23, 0);
        voice_trans_frame_t new_frame;
        ima_adpcm_encoder(evt->data, new_frame.encode_data, MAX_VOICE_WORD_SIZE);
        // ima_adpcm_encoder(test_pcm_data, new_frame.encode_data, MAX_VOICE_WORD_SIZE);
        // memcpy(new_frame.encode_data, test_pcm_data, 128);
        // memcpy(new_frame.encode_data, evt->data, 128);
        new_frame.seq_id = trans_seq_id++;
        //test
        // ima_adpcm_global_state.index = 2;
        // ima_adpcm_global_state.valprev = 3;
        new_frame.state = ima_adpcm_global_state;
        if (voice_fifo_in(&new_frame) == false) {
            LOGE("VOICE", "voice fifo full");
            phy_voice_stop();
        }
        app_event_set(APP_EVENT_VOICE);
        phy_gpio_write(GPIO_P23, 1);
#endif
    } else {
        return;
    }
}

bool  voice_init()
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
    cfg.voiceRate = VOICE_RATE_8K;
    cfg.voiceAutoMuteOnOff = false;
    phy_voice_config(cfg, voice_pcm_truck_cb);

    phy_gpio_pin_init(GPIO_P23, OEN);
    phy_gpio_pull_set(GPIO_P23, WEAK_PULL_UP);

    return true;
}


bool voice_handle_mic_start()
{
    if (is_voice_busy) {
        LOGE("VOICE", "voice is working");
        return false;
    }

    voice_init();
#if (VOICE_ENC_TYPE == SW_IMA_ADPCM_ENC) 
    ima_adpcm_global_state.index = 0;
    ima_adpcm_global_state.valprev = 0;
#endif
    is_voice_busy = true;
    trans_seq_id = 0;

    voice_fifo.head = 0;
    voice_fifo.tail = 0;

    phy_voice_start();
    disableSleepInPM(0x04);

    phy_gpio_write(GPIO_P23, 0);
    return true;
}

bool voice_handle_mic_stop()
{
    if(is_voice_busy == false) {
        return false;
    }
    phy_voice_stop();
    enableSleepInPM(0x04);
    is_voice_busy = false;

    phy_gpio_write(GPIO_P23, 1);
    return true;
}


// void voice_handle_new_data(short *pcm_raw_data, uint16_t len)
// {
//     // 2. 编码
//     // ima_adpcm_encoder(voice_raw_pcm, new_frame.encode_data, 256, &ima_adpcm_global_state);
//     // new_frame.seq_id = trans_seq_id;
//     // new_frame.state = ima_adpcm_global_state;
//     // 3.FIFO入列
//     // if (voice_fifo_in(&new_frame) == false) {
//     //     // TO-DO: FIFO已满无法入列，语音帧丢包
//     //     // LOGE("VOICE", "vocice FIFO full error");
//     // }
//     // p_trans_frame->seq_id = trans_seq_id;
//     // p_trans_frame->state = ima_adpcm_global_state;
//     // 4. 发送通知
//     voice_trans_frame_t new_frame;
//     // ima_adpcm_encoder(pcm_raw_data, new_frame.encode_data, MAX_VOICE_WORD_SIZE, &ima_adpcm_global_state);
//     memcpy(new_frame.encode_data, pcm_raw_data, 128);
//     new_frame.seq_id = trans_seq_id;
//     // new_frame.state = ima_adpcm_global_state;
//     new_frame.state.valprev = (uint16_t)pcm_raw_data;
    

//     if (voice_fifo_in(&new_frame) == false) {
//         // 入列错误
//         // LOGE("VOICE", "VOICE FIFO in failed");
//     }
// }

/*********************************************************************************
 * FIFO 管理
 ********************************************************************************/
bool voice_fifo_is_empty()
{
    if (voice_fifo.head == voice_fifo.tail) {
        return true;
    } else {
        return false;
    }
}

bool voice_fifo_is_full()
{
    if (((voice_fifo.tail + 1) % VOICE_FIFO_MAX_SIZE) == voice_fifo.head)
    {
        return true;
    } else {
        return false;
    }
}

bool voice_fifo_top(voice_trans_frame_t *out_frame)
{
    if (voice_fifo_is_empty() == false) {
        uint16_t top = (voice_fifo.head + 1) % VOICE_FIFO_MAX_SIZE;
        memcpy(out_frame, &voice_fifo.frames[top], sizeof(voice_trans_frame_t));
        // return &voice_fifo.frames[voice_fifo.head];
        return true;
    } else {
        return false;
    }
}

void voice_fifo_pop()
{
    voice_fifo.head = (voice_fifo.head + 1) % VOICE_FIFO_MAX_SIZE;
}

bool voice_fifo_out(voice_trans_frame_t *out_frame)
{
    if (voice_fifo_is_empty() == false) {
        voice_fifo.head = (voice_fifo.head + 1) % VOICE_FIFO_MAX_SIZE;
        memcpy(out_frame, &voice_fifo.frames[voice_fifo.head], sizeof(voice_trans_frame_t));
        // return &voice_fifo.frames[voice_fifo.head];
        return true;
    } else {
        return false;
    }
}


bool voice_fifo_in(voice_trans_frame_t * in_frame)
{
    if (voice_fifo_is_full() == false) {
        voice_fifo.tail = (voice_fifo.tail + 1) % VOICE_FIFO_MAX_SIZE;
        memcpy(&voice_fifo.frames[voice_fifo.tail], in_frame, sizeof(voice_trans_frame_t));
        // return &voice_fifo.frames[voice_fifo.tail];
        return true;
    } else {
        return false;
    }
}

int voice_fifo_flush();
int voice_fifo_size();