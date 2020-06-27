#include <trace.h>
#include <string.h>
#include "board.h"
#include "voice.h"
#include "voice_driver.h"
#include "profile_server.h"
#include "hids_rmc.h"
#include "profile_server.h"
#include "app_msg.h"
#include "rcu_application.h"

#include "swtimer.h"
#include "key_handle.h"
#include "os_timer.h"
#include "rcu_dfu_service.h"

#if (VOICE_ENC_TYPE == SW_SBC_ENC)
#include "sbc.h"
#elif (VOICE_ENC_TYPE == SW_IMA_ADPCM_ENC)
#include "ima_adpcm_enc.h"
#elif (VOICE_ENC_TYPE == SW_OPT_ADPCM_ENC)
#include "voice_adpcm.h"
#endif

#if (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
#include "atvv_service.h"
#endif

#if RCU_VOICE_EN
/*============================================================================*
 *                              Local Variables
 *============================================================================*/
#if (VOICE_ENC_TYPE == SW_MSBC_ENC)
static uint8_t msbc_tog = 0;
#endif
uint8_t voice_long_press_flag = 0;
uint8_t voice_short_release_flag = 0;
uint8_t pre_voice_need_send_nc_flag = 1;
T_VOICE_MODE_TYPE voice_mode = VOICE_MODE_NULL;
static uint8_t voice_queue_buffer[VOICE_REPORT_FRAME_SIZE * VOICE_QUEUE_MAX_LENGTH] = {0};
static T_VOICE_QUEUE_DEF voice_queue;
#if (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
/* voice_exception_timer is used to detect voice process exception */
static TimerHandle_t voice_exception_timer = NULL;
#endif

/*============================================================================*
 *                              Global Variables
 *============================================================================*/
T_VOICE_GLOBAL_DATA voice_global_data;

/*============================================================================*
 *                              External Functions
 *============================================================================*/
#if (VOICE_ENC_TYPE == SW_MSBC_ENC)
extern void msbc_init(void);
extern void msbc_deinit(void);
extern int msbc_encode(void *input, int input_len, void *output, int output_len, int *written);
#endif

/*============================================================================*
 *                              Loacal Functions
 *============================================================================*/
static void voice_handle_init_data(void);
static void voice_handle_init_encode_param(void);
static void voice_handle_deinit_encode_param(void);
static void voice_handle_init_queue(void);
static bool voice_handle_is_queue_full(void);
static bool voice_handle_is_queue_empty(void);
static uint32_t voice_handle_get_queue_item_cnt(void);
static bool voice_handle_in_queue(uint8_t *buffer);
static bool voice_handle_out_queue(void);
static void voice_handle_encode_raw_data(uint8_t *p_input_data, int32_t input_size,
                                         uint8_t *p_output_data, int32_t *p_output_size);
static void voice_handle_gdma_event(T_IO_MSG io_driver_msg_recv);
#if (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
static void voice_handle_exception_timer_cb(TimerHandle_t p_timer);
#endif

/**
* @brief  Initialize voice handle data
*/
void voice_handle_init_data(void)
{
    APP_PRINT_INFO0("[voice_handle_init_data] init data");
    memset(&voice_global_data, 0, sizeof(voice_global_data));

    voice_handle_init_queue();
}

/**
 * @brief init voice encode parameters.
 * @param none
 * @return none
 */
void voice_handle_init_encode_param(void)
{
#if (VOICE_ENC_TYPE == SW_MSBC_ENC)
    msbc_init();
#elif (VOICE_ENC_TYPE == SW_SBC_ENC)
    sbc_enc_params.samplingFrequency = SBC_FREQU16000;
    sbc_enc_params.blockNumber = SBC_BLOCKS16;
    sbc_enc_params.channelMode = SBC_MODE_MONO;
    sbc_enc_params.allocMethod = SBC_ALLOCLOUDNESS;
    sbc_enc_params.subbandNumber = SBC_SUBBANDS8;
    sbc_enc_params.bitpool = BIT_POOL_SIZE;
    sbc_init_encoder();
#elif (VOICE_ENC_TYPE == SW_IMA_ADPCM_ENC)
    memset(&ima_adpcm_global_state, 0, sizeof(ima_adpcm_global_state));
#endif
}

/**
 * @brief deinit voice encode parameters.
 * @param none
 * @return none
 */
void voice_handle_deinit_encode_param(void)
{
#if (VOICE_ENC_TYPE == SW_MSBC_ENC)
    msbc_deinit();
#endif
}

/**
 * @brief voice handle init queue
 * @param none
 * @return none
 */
void voice_handle_init_queue(void)
{
    voice_queue.queue_size = VOICE_QUEUE_MAX_LENGTH;
    voice_queue.out_queue_index   = 0;
    voice_queue.in_queue_index  = 0;
    voice_queue.p_voice_buff = (uint8_t *)voice_queue_buffer;
}

/**
 * @brief check if voice queue is full
 * @param none
 * @return true or false
 */
bool voice_handle_is_queue_full(void)
{
    return ((voice_queue.in_queue_index + 1) % voice_queue.queue_size == voice_queue.out_queue_index);
}

/**
 * @brief check if voice queue is empty
 * @param none
 * @return true or false
 */
bool voice_handle_is_queue_empty(void)
{
    return (voice_queue.in_queue_index == voice_queue.out_queue_index);
}

/**
 * @brief voice handle get queue item count
 * @param none
 * @return item count
 */
uint32_t voice_handle_get_queue_item_cnt(void)
{
    if (voice_queue.in_queue_index >= voice_queue.out_queue_index)
    {
        return (voice_queue.in_queue_index - voice_queue.out_queue_index);
    }
    else
    {
        return (voice_queue.in_queue_index + voice_queue.queue_size - voice_queue.out_queue_index);
    }
}

/**
 * @brief voice handle in queue
 * @param buffer - buffer data to be stored
 * @return true or false
 */
bool voice_handle_in_queue(uint8_t *buffer)
{
    if (voice_handle_is_queue_full())
    {
        APP_PRINT_INFO0("[voice_handle_in_queue] queue is full, drop oldest data");
        voice_queue.out_queue_index = (voice_queue.out_queue_index + 1) % voice_queue.queue_size;
    }
    memcpy(voice_queue.p_voice_buff + voice_queue.in_queue_index * VOICE_REPORT_FRAME_SIZE,
           (uint8_t *)buffer,
           VOICE_REPORT_FRAME_SIZE);
    voice_queue.in_queue_index = (voice_queue.in_queue_index + 1) % voice_queue.queue_size;

    return true;
}

/**
 * @brief voice handle in queue
 * @param none
 * @return true or false
 */
bool voice_handle_out_queue(void)
{
    bool result = false;
    uint8_t gap_link_credits = 0;

#if (VOICE_FLOW_SEL == IFLYTEK_VOICE_FLOW)
    uint8_t reserved_credits = 1;
#elif (VOICE_FLOW_SEL == HIDS_GOOGLE_VOICE_FLOW)
    uint8_t reserved_credits = 2;
#elif (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
    uint8_t reserved_credits = 1;
#endif

    le_get_gap_param(GAP_PARAM_LE_REMAIN_CREDITS, &gap_link_credits);

    if (voice_handle_is_queue_empty() == true)
    {
        APP_PRINT_INFO0("[voice_handle_out_queue] Voice Queue is empty.");
        result = false;
    }
    else if (app_global_data.mtu_size - 3 < VOICE_REPORT_FRAME_SIZE)
    {
        APP_PRINT_WARN1("[voice_handle_out_queue] mtu size() is too small!", app_global_data.mtu_size);
        result = false;
    }
    else if (gap_link_credits <=
             reserved_credits)  /* reserve at least one notification FIFO for key event */
    {
        APP_PRINT_WARN1("[voice_handle_out_queue] gap_link_credits() is not enough!", gap_link_credits);
        result = false;
    }
    else
    {
        uint32_t queue_item_cnt = voice_handle_get_queue_item_cnt();
        uint32_t loop_cnt = (queue_item_cnt <= gap_link_credits - 1) ? queue_item_cnt :
                            (gap_link_credits - 1);

        APP_PRINT_INFO3("[voice_handle_out_queue] gap_link_credits is %d, queue_item_cnt is %d, loop_cnt is %d",
                        gap_link_credits, queue_item_cnt, loop_cnt);

        while ((loop_cnt > 0) && (voice_handle_is_queue_empty() == false))
        {
            loop_cnt--;
            /* attampt to send voice data */
#if (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
            result = server_send_data(0, app_global_data.atvv_srv_id, GATT_SVC_ATVV_CHAR_RX_VALUE_INDEX, \
                                      voice_queue.p_voice_buff + voice_queue.out_queue_index * VOICE_REPORT_FRAME_SIZE, \
                                      VOICE_REPORT_FRAME_SIZE, GATT_PDU_TYPE_NOTIFICATION);
#else
            result = server_send_data(0, app_global_data.hid_srv_id, GATT_SRV_HID_VOICE_INPUT_INDEX, \
                                      voice_queue.p_voice_buff + voice_queue.out_queue_index * VOICE_REPORT_FRAME_SIZE, \
                                      VOICE_REPORT_FRAME_SIZE, GATT_PDU_TYPE_NOTIFICATION);
#endif
            if (result == true)
            {
                /* send successfully, increase out_queue_index */
                voice_queue.out_queue_index = (voice_queue.out_queue_index + 1) % voice_queue.queue_size;
            }
            else
            {
                APP_PRINT_WARN0("[voice_handle_out_queue] server_send_data failed");
                break;
            }
        }
    }

    return result;
}

/**
* @brief   voice handle start mic and recording
* @param   No parameter.
* @return  true or false
*/
bool voice_handle_start_mic(void)
{
    app_global_data.is_voice_able_to_check_bat_vol = false;
    if (voice_driver_global_data.is_voice_driver_working == true)
    {
        APP_PRINT_INFO0("[voice_handle_start_mic] Voice driver is working, start failed!");
        return false;
    }

    APP_PRINT_INFO0("[voice_handle_start_mic] start recording!");

    voice_handle_init_data();
    voice_handle_init_encode_param();
    app_set_latency_status(LANTENCY_OFF);  /* off latency to speed up voice process */
    voice_driver_init();

    return true;
}

/**
* @brief   voice handle stop mic and recording
* @param   No parameter.
* @return  true or false
*/
void voice_handle_stop_mic(void)
{
    APP_PRINT_INFO0("[voice_handle_stop_mic] stop recording!");

#if SUPPORT_SILENT_OTA
    if (!dfu_check_working())
#endif
    {
        app_set_latency_status(LANTENCY_ON);
    }

#if (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
    if (voice_exception_timer)
    {
        os_timer_stop(&voice_exception_timer);
    }
#endif

    if (voice_driver_global_data.is_voice_driver_working == false)
    {
        APP_PRINT_INFO0("[voice_handle_stop_mic] Voice driver is not working, stop failed!");
        if (voice_mode == VOICE_MODE_PRE)
        {
            voice_long_press_flag = 0;
            voice_short_release_flag = 0;
        }
    }
    else
    {
        voice_long_press_flag = 0;
        voice_short_release_flag = 0;
        voice_driver_deinit();
        voice_handle_deinit_encode_param();
    }
    app_global_data.is_voice_able_to_check_bat_vol = true;
}

/**
* @brief   voice handle encode raw data
* @param   p_input_data - point of input data, input_size - input size,
*          p_output_data - point of output data, p_output_size - point of output size
* @return  void
*/
void voice_handle_encode_raw_data(uint8_t *p_input_data, int32_t input_size,
                                  uint8_t *p_output_data, int32_t *p_output_size)
{
#if (VOICE_ENC_TYPE == SW_MSBC_ENC)
    int encoded = 0;
    msbc_encode(p_input_data, VOICE_PCM_FRAME_SIZE, (uint8_t *)p_output_data + 2,
                VOICE_FRAME_SIZE_AFTER_ENC, &encoded);

    p_output_data[0] = 0x1;
    p_output_data[59] = 0x00;
    if ((msbc_tog & 0x1) == 0)
    {
        p_output_data[1] = 0x8;
    }
    else
    {
        p_output_data[1] = 0xc8;
    }

    msbc_encode(p_input_data + 240, VOICE_PCM_FRAME_SIZE,
                (uint8_t *)p_output_data + 62,
                VOICE_FRAME_SIZE_AFTER_ENC, &encoded);

    p_output_data[60] = 0x1;
    p_output_data[119] = 0x00;
    if ((msbc_tog & 0x1) == 0)
    {
        p_output_data[61] = 0x38;
    }
    else
    {
        p_output_data[61] = 0xf8;
    }
    *p_output_size = 120;
    msbc_tog++;

#elif (VOICE_ENC_TYPE == SW_SBC_ENC)
    uint8_t *p_input_buff = p_input_data;
    uint8_t *p_output_buff = p_output_data;
    int32_t tmp_output_size;
    while (input_size > 0)
    {
        tmp_output_size = VOICE_FRAME_SIZE_AFTER_ENC;
        if (input_size > VOICE_PCM_FRAME_SIZE)
        {
            sbc_encode(p_input_buff, VOICE_PCM_FRAME_SIZE, &sbc_enc_params, p_output_buff, &tmp_output_size);
            p_input_buff += VOICE_PCM_FRAME_SIZE;
            p_output_buff += tmp_output_size;
            *p_output_size += tmp_output_size;
            input_size -= VOICE_PCM_FRAME_SIZE;
        }
        else
        {
            sbc_encode(p_input_buff, input_size, &sbc_enc_params, p_output_buff, &tmp_output_size);

            *p_output_size += tmp_output_size;
            input_size = 0;
        }
    }
#elif (VOICE_ENC_TYPE == SW_IMA_ADPCM_ENC)
    int32_t tmp_size;
    p_output_data[0] = (uint8_t)(ima_adpcm_global_state.seq_id >> 8);
    p_output_data[1] = (uint8_t)(ima_adpcm_global_state.seq_id);
    ima_adpcm_global_state.seq_id++;
    p_output_data[2] = 0;
    p_output_data[3] = (uint8_t)(ima_adpcm_global_state.valprev >> 8);
    p_output_data[4] = (uint8_t) ima_adpcm_global_state.valprev;
    p_output_data[5] = ima_adpcm_global_state.index;

    tmp_size = ima_adpcm_encode((void *)p_input_data, &p_output_data[6], VOICE_PCM_FRAME_SIZE / 2,
                                &ima_adpcm_global_state);
    *p_output_size = tmp_size + 6;

    APP_PRINT_INFO1("[voice_handle_encode_raw_data] *p_output_size = %d", *p_output_size);
#elif (VOICE_ENC_TYPE == SW_OPT_ADPCM_ENC)
    for (uint8_t i = 0; i < (VOICE_GDMA_FRAME_SIZE / VOICE_SAMPLE_NUM / 2) ; i++)
    {
        gAdpcmEncState.inp = (int16_t *)p_input_data + VOICE_SAMPLE_NUM * i;
        gAdpcmEncState.out = p_output_data;
        gAdpcmEncState.len = VOICE_SAMPLE_NUM;
        gAdpcmEncState.imaSize = VOICE_IMG_BITS;
        gAdpcmEncState.imaOr = IMG_OR;
        gAdpcmEncState.imaAnd = IMG_AND;
        Adpcm_Coder(&gAdpcmEncState);
        *p_output_size = VOICE_REPORT_FRAME_SIZE;
    }
#endif
}

/**
* @brief   voice handle GDMA evnet
* @param   io_driver_msg_recv - gdma message
* @return  void
*/
void voice_handle_gdma_event(T_IO_MSG io_driver_msg_recv)
{
    int32_t output_size = 0;
    uint8_t encode_output_buffer[VOICE_REPORT_FRAME_SIZE];

    if (true == voice_global_data.is_pending_to_stop_recording)
    {
        if ((voice_handle_is_queue_empty() == true) ||
            (voice_global_data.is_allowed_to_notify_voice_data == false))
        {
            /* voice buffer data has all been sent after voice key up */
#if (VOICE_FLOW_SEL == IFLYTEK_VOICE_FLOW)
            key_handle_notfiy_key_data(VK_NC);
            voice_handle_stop_mic();
#elif (VOICE_FLOW_SEL == HIDS_GOOGLE_VOICE_FLOW)
            voice_global_data.is_allowed_to_notify_voice_data = false;
            voice_mode = VOICE_MODE_NULL;
            //key_handle_notfiy_key_data(VK_VOICE_STOP);
            key_handle_notfiy_key_data(VK_NC);
            voice_handle_stop_mic();
#endif
        }
        else
        {
            voice_handle_out_queue();
        }
    }
    else
    {
        /* encode raw data */
        voice_handle_encode_raw_data(io_driver_msg_recv.u.buf, VOICE_GDMA_FRAME_SIZE, encode_output_buffer,
                                     &output_size);

        if (output_size == VOICE_REPORT_FRAME_SIZE)
        {
            voice_handle_in_queue((uint8_t *)encode_output_buffer);

            if (voice_global_data.is_allowed_to_notify_voice_data == true)
            {
                voice_handle_out_queue();
            }
            else
            {
                APP_PRINT_INFO0("[voice_handle_gdma_event] not allowed to notify voice data");
            }
        }
        else
        {
            APP_PRINT_WARN1("[voice_handle_gdma_event] encode failed, output size is %d", output_size);
        }
    }
}

#if (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
/**
 * @brief    voice exception timer callback
 * @param    p_timer - point of timer
 * @return   none
 * @retval   void
 * Caution   do NOT excute time consumption functions in timer callback
 */
void voice_handle_exception_timer_cb(TimerHandle_t p_timer)
{
    APP_PRINT_INFO0("[voice_handle_exception_timer_cb] timeout");

    voice_handle_stop_mic();
}
#endif

/*============================================================================*
 *                              Global Functions
 *============================================================================*/
void voice_timer_callback(TimerHandle_t p_timer)
{
    APP_PRINT_INFO0("[voice_timer_callback] voice_timer_callback!");
    if ((voice_long_press_flag == 0) && (voice_short_release_flag == 0))
    {
        if ((voice_global_data.is_allowed_to_notify_voice_data) || (voice_mode == VOICE_MODE_PRE))
        {
            APP_PRINT_INFO0("[voice_timer_callback] Voice press time is over 30ms!");
            voice_long_press_flag = 1;
        }
        os_timer_restart(&voice_timer, VOICE_STOP_TIMEOUT);

    }
    else if ((voice_long_press_flag == 1) || (voice_short_release_flag == 1))
    {
        APP_PRINT_INFO0("[voice_timer_callback] Voice recording time is over 30s, stop voice!");
        voice_global_data.is_allowed_to_notify_voice_data = false;
        voice_mode = VOICE_MODE_NULL;
        voice_handle_stop_mic();
        key_handle_notfiy_key_data(VK_NC);
    }
}

void pre_voice_send_nc_timer_callback(TimerHandle_t p_timer)
{
    APP_PRINT_INFO0("[pre_voice_send_nc_timer_callback] pre_voice_send_nc_timer_callback!");
    key_handle_notfiy_key_data(VK_NC);
}

/**
* @brief vocie key press msg handle.
* @param   No parameter.
* @return  void
*/
bool voice_handle_mic_key_pressed(void)
{
    bool ret = true;

    key_handle_notfiy_key_data(VK_VOICE);

    if (true == voice_driver_global_data.is_voice_driver_working)
    {
        APP_PRINT_WARN0("[voice_handle_mic_key_pressed] Voice driver is Working, start recording failed!");
        return false;
    }

    if (voice_mode == VOICE_MODE_NORMAL)
    {
        APP_PRINT_WARN0("[voice_handle_mic_key_pressed] restart voice_timer 30ms!");
        os_timer_restart(&voice_timer, VOICE_PRESS_TIMEOUT);
        voice_long_press_flag = 0;
        voice_short_release_flag = 0;
    }

#if (VOICE_FLOW_SEL == IFLYTEK_VOICE_FLOW)
    voice_handle_start_mic();
    key_handle_notfiy_key_data(VK_VOICE);
#elif (VOICE_FLOW_SEL == HIDS_GOOGLE_VOICE_FLOW)
    voice_handle_start_mic();
    //key_handle_notfiy_key_data(VK_VOICE);
    //key_handle_notfiy_key_data(VK_NC);
    voice_global_data.is_allowed_to_notify_voice_data = true;
#elif (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
    /* send SEARCH_KEY and modify last_pressed_key_index */
    key_handle_notfiy_key_data(MM_AC_Search);
    key_handle_global_data.last_pressed_key_index = MM_AC_Search;

    /* send ATV_CTL_OPCODE_START_SEARCH */
    memset(atvv_global_data.char_ctl_data_buff, 0, ATVV_CHAR_CTL_DATA_LEN);
    atvv_global_data.char_ctl_data_buff[0] = ATV_CTL_OPCODE_START_SEARCH;
    server_send_data(0, app_global_data.atvv_srv_id, GATT_SVC_ATVV_CHAR_CTL_VALUE_INDEX, \
                     atvv_global_data.char_ctl_data_buff, sizeof(atvv_global_data.char_ctl_data_buff),
                     GATT_PDU_TYPE_NOTIFICATION);
    app_set_latency_status(LANTENCY_OFF);  /* off latency to speed up voice process */
    /* start voice exception timer */
    if (voice_exception_timer == NULL)
    {
        if (true == os_timer_create(&voice_exception_timer, "voice_exception_timer",  1, \
                                    VOICE_EXCEPTION_TIMEOUT, false, voice_handle_exception_timer_cb))
        {
            os_timer_start(&voice_exception_timer);
        }
        else
        {
            APP_PRINT_ERROR0("[voice_handle_mic_key_pressed] voice_exception_timer creat failed!");
        }
    }
    else
    {
        os_timer_start(&voice_exception_timer);
    }
#endif
    return ret;
}

/**
* @brief vocie key release msg handle.
* @param   No parameter.
* @return  void
*/
void voice_handle_mic_key_released(void)
{
    APP_PRINT_WARN0("[voice_handle_mic_key_released] voice_handle_mic_key_released!");
    APP_PRINT_WARN1("[voice_handle_mic_key_released] voice_mode =  %d", voice_mode);
    if (voice_mode == VOICE_MODE_NORMAL)
    {
        if (voice_driver_global_data.is_voice_driver_working == false)
        {
            APP_PRINT_WARN0("[voice_handle_mic_key_released] VOICE_MODE_NORMAL Voice driver is not working!");
            return;
        }
    }

    if (voice_long_press_flag == 0)
    {
        if (voice_short_release_flag == 0)
        {
            os_timer_restart(&voice_timer, VOICE_STOP_TIMEOUT);
            voice_short_release_flag = 1;
            if (voice_mode == VOICE_MODE_NORMAL)
            {
                key_handle_notfiy_key_data(VK_NC);
            }
            else if (voice_mode == VOICE_MODE_PRE)
            {
                pre_voice_need_send_nc_flag = 1;
                if ((app_global_data.rcu_status == RCU_STATUS_PAIRED) && (app_global_data.is_keyboard_notify_en))
                {
                    pre_voice_need_send_nc_flag = 0;
                    key_handle_notfiy_key_data(VK_NC);
                }
            }
        }
        return;
    }

    os_timer_stop(&voice_timer);

    if (voice_mode == VOICE_MODE_PRE)
    {
        voice_mode = VOICE_MODE_NULL;
        if (voice_driver_global_data.is_voice_driver_working == false)
        {
            APP_PRINT_WARN0("[voice_handle_mic_key_released] VOICE_MODE_PRE Voice driver is not working!");
            return;
        }
    }

#if (VOICE_FLOW_SEL == IFLYTEK_VOICE_FLOW)
    if (voice_handle_is_queue_empty() ||
        (false == voice_global_data.is_allowed_to_notify_voice_data) ||
        (app_global_data.mtu_size - 3 < VOICE_REPORT_FRAME_SIZE))
    {
        /* stop voice recording immediately */
        key_handle_notfiy_key_data(VK_NC);
        voice_handle_stop_mic();
    }
    else
    {
        /* stop voice in queue, and wait for all buffered voice data send */
        voice_global_data.is_pending_to_stop_recording = true;
    }
#elif (VOICE_FLOW_SEL == HIDS_GOOGLE_VOICE_FLOW)
    if (voice_handle_is_queue_empty() ||
        (false == voice_global_data.is_allowed_to_notify_voice_data) ||
        (app_global_data.mtu_size - 3 < VOICE_REPORT_FRAME_SIZE))
    {
        voice_global_data.is_allowed_to_notify_voice_data = false;
        /* stop voice recording immediately */
        //key_handle_notfiy_key_data(VK_VOICE_STOP);
        voice_mode = VOICE_MODE_NULL;
        key_handle_notfiy_key_data(VK_NC);
        voice_handle_stop_mic();
    }
    else
    {
        /* stop voice in queue, and wait for all buffered voice data send */
        voice_global_data.is_pending_to_stop_recording = true;
    }
#elif (VOICE_FLOW_SEL == ATV_GOOGLE_VOICE_FLOW)
    key_handle_notfiy_key_data(VK_NC);
#endif
}

/**
* @brief   voice module msg handle.
* @param   T_VOICE_MSG_TYPE msg_type, void *p_data.
* @return  bool.
*/
bool voice_handle_messages(T_VOICE_MSG_TYPE msg_type, void *p_data)
{
    bool ret = true;

    APP_PRINT_INFO1("[voice_handle_messages] msg_type is %d", msg_type);

    switch (msg_type)
    {
    case VOICE_MSG_PERIPHERAL_GDMA:
        {
            voice_handle_gdma_event(*(T_IO_MSG *)p_data);
        }
        break;
    case VOICE_MSG_BT_SEND_COMPLETE:
        {
            if (voice_global_data.is_allowed_to_notify_voice_data)
            {
                voice_handle_out_queue();
            }
        }
        break;
    case VOICE_MSG_BT_WRITE_CMD:
        {
            uint8_t cmd_type = *(uint8_t *)p_data;
            if (cmd_type == 0x01)
            {
                APP_PRINT_INFO1("[voice_handle_messages] Enable voice data notification", cmd_type);
                voice_mode = VOICE_MODE_NORMAL;
                voice_handle_start_mic();
                voice_global_data.is_allowed_to_notify_voice_data = true;
            }
            else if (cmd_type == 0x00)
            {
                APP_PRINT_INFO1("[voice_handle_messages] Disable voice data notification", cmd_type);
                voice_global_data.is_allowed_to_notify_voice_data = false;
                voice_mode = VOICE_MODE_NULL;
                os_timer_stop(&voice_timer);
                if (voice_driver_global_data.is_voice_driver_working == true)
                {
                    voice_handle_stop_mic();
                }
            }
            else
            {
                APP_PRINT_INFO1("[voice_handle_messages] VOICE_MSG_BT_WRITE_CMD unknow cmd %d", cmd_type);
            }
        }
        break;

    default:
        APP_PRINT_INFO0("[voice_handle_messages] unknown msg type!");
        ret = false;
        break;
    }
    return ret;
}

#endif


