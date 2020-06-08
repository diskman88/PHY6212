#include <csi_config.h>
#ifdef CONFIG_KERNEL_UCOS
#include "trcKernelPort_ucos.c"
#elif CONFIG_KERNEL_FREERTOS
#include "trcKernelPort_freertos.c"
#elif CONFIG_KERNEL_RHINO
#include "trcKernelPort_rhino.c"
#endif
