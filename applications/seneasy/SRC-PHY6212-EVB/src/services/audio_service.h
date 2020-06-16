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

#include "../board/ch6121_evb/msg.h"
#include "../board/ch6121_evb/event.h"

void audio_service_init();

void audio_start_record();

#endif
