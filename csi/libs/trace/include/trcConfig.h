#ifndef __TRC_CONFIG_H__
#define __TRC_CONFIG_H__

#ifdef CONFIG_KERNEL_UCOS
#include "trcConfig_ucos.h"
#elif CONFIG_KERNEL_FREERTOS
#include "trcConfig_freertos.h"
#elif CONFIG_KERNEL_RHINO
#include "trcConfig_rhino.h"
#endif

#endif
