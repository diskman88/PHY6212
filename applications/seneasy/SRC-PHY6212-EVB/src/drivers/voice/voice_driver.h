
#ifndef _VOICE_HANDLE_H_
#define _VOICE_HANDLE_H_

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include "ima_adpcm.h"
#include "voice.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Macros
 *============================================================================*/
#define VOICE_PCM_DATA_LEN  MAX_VOICE_WORD_SIZE
#define VOICE_FIFO_MAX_SIZE 50  /* voice queue length based on send units */

#define SW_SBC_ENC          0
#define SW_IMA_ADPCM_ENC    1
#define SW_OPT_ADPCM_ENC    2

#define VOICE_ENC_TYPE  SW_IMA_ADPCM_ENC 

/*============================================================================*
 *                         Types
 *============================================================================*/
/*voice msg type*/
typedef enum
{
    VOICE_MSG_INVALID           = 0,
    VOICE_MSG_BT_SEND_COMPLETE  = 1,
    VOICE_MSG_BT_WRITE_CMD      = 2,
    VOICE_MSG_PERIPHERAL_NEW_FRAME   = 3,
} VOICE_MSG_T;


#if (VOICE_ENC_TYPE == SW_IMA_ADPCM_ENC)
typedef struct 
{
    // 传输id
    uint16_t seq_id;
    char reserved;
    // adpcm编码
    adpcm_state_t state;        // adpcm编码状态
    char encode_data[128];       // 音频编码数据
}voice_trans_frame_t;
#endif

typedef struct
{
    uint16_t head;
    uint16_t tail;
    // uint16_t data[PCM_FIFO_SIZE];
    voice_trans_frame_t frames[VOICE_FIFO_MAX_SIZE];
} voice_fifo_def_t;


/*============================================================================*
*                        Export Global Variables
*============================================================================*/
// extern uint8_t voice_long_press_flag;
// extern uint8_t voice_short_release_flag;
// extern uint8_t pre_voice_need_send_nc_flag;
// extern T_VOICE_MODE_TYPE voice_mode;
extern bool is_voice_busy;
/*============================================================================*
 *                         Functions
 *============================================================================*/
// bool voice_handle_mic_key_pressed(void);
// void voice_handle_mic_key_released(void);
typedef void(* voice_new_frame_callback_t)(void);


bool voice_init();

bool voice_handle_mic_start();

bool voice_handle_mic_stop();

void voice_handle_new_data(short *pcm_raw_data, uint16_t len);

bool voice_fifo_top(voice_trans_frame_t *out_frame);
void voice_fifo_pop();

bool voice_fifo_out(voice_trans_frame_t *out_frame);

bool voice_fifo_in(voice_trans_frame_t * in_frame);

int voice_fifo_flush();

int voice_fifo_size();

#ifdef __cplusplus
}
#endif

#endif
