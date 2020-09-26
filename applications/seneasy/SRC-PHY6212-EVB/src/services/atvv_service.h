#ifndef _AUDIO_SERVICE_H
#define _AUDIO_SERVICE_H
#include <stdint.h>
#include <stdbool.h>
#include <drv_common.h>
#include <pinmux.h>
#include <csi_config.h>
#include <drv_irq.h>
#include <drv_gpio.h>
#include <drv_pmu.h>
#include <drv_adc.h>
#include <dw_gpio.h>
#include <pin_name.h>
#include <clock.h>
#include <soc.h>
#include <yoc/cli.h>
#include <aos/cli.h>
#include <pm.h>
#include <voice.h>

typedef struct atvv_service_t
{
    uint16 svc_handle;
    uint16 char_rx_cccd;
    uint16 char_ctl_cccd;
}atvv_service_t;

atvv_service_t * atvv_service_init();

int atvv_voice_send(const uint8_t * p_voice_data, uint16_t len);

int atvv_ctl_send(const uint8_t * p_data, uint16_t len);

#endif
