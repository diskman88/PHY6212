/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     voice.h
* @brief    This is the header file of user code which the voice module resides in.
* @details
* @author   chenjie jin
* @date     2018-05-03
* @version  v1.1
*********************************************************************************************************
*/

#ifndef _VOICE_HANDLE_H_
#define _VOICE_HANDLE_H_

/*============================================================================*
 *                        Header Files
 *============================================================================*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              Macros
 *============================================================================*/
#define VOICE_QUEUE_MAX_LENGTH 60  /* voice queue length based on send units */

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

typedef enum
{
    VOICE_MODE_NULL             = 0,
    VOICE_MODE_NORMAL           = 1,
    VOICE_MODE_PRE              = 2,
} T_VOICE_MODE_TYPE;

typedef struct
{
    uint32_t queue_size;
    uint32_t out_queue_index;
    uint32_t in_queue_index;
    uint8_t *p_voice_buff;
} T_VOICE_QUEUE_DEF;

typedef struct
{
    bool is_allowed_to_notify_voice_data;  /* to indicate whether is allowed to notify voice data or not */
    bool is_pending_to_stop_recording;   /* indicate whether is pending to stop recording */
} T_VOICE_GLOBAL_DATA;

/*============================================================================*
*                        Export Global Variables
*============================================================================*/
extern T_VOICE_GLOBAL_DATA voice_global_data;
extern uint8_t voice_long_press_flag;
extern uint8_t voice_short_release_flag;
extern uint8_t pre_voice_need_send_nc_flag;
extern T_VOICE_MODE_TYPE voice_mode;
/*============================================================================*
 *                         Functions
 *============================================================================*/
bool voice_handle_mic_key_pressed(void);
void voice_handle_mic_key_released(void);
bool voice_handle_messages(T_VOICE_MSG_TYPE msg_type, void *p_data);
bool voice_handle_start_mic(void);
void voice_handle_stop_mic(void);

#ifdef __cplusplus
}
#endif

#endif
