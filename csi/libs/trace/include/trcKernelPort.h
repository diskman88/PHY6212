#include <csi_config.h>

#ifdef CONFIG_KERNEL_UCOS
#include "trcKernelPort_ucos.h"
#elif CONFIG_KERNEL_FREERTOS
#include "trcKernelPort_freertos.h"
#elif CONFIG_KERNEL_RHINO
#include "trcKernelPort_rhino.h"
#endif
