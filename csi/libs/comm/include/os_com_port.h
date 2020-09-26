#ifndef __OS_COM_KERNEL_PORT_H__
#define __OS_COM_KERNEL_PORT_H__

#include "os_com_cfg.h"
#include <os_port.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The Followings are RS232 port interfaces.  */

/* Init RS232 as the configs in os_com_cfg.h.  */
extern int32_t os_com_rs232_init(void);

/* RS232 Transmit data.  */
extern int32_t os_com_rs232_transmit(uint8_t*pucData, uint32_t ulNumBytes);

/* Get transmit function from communication type.  */
extern os_com_service_tx_func_t os_com_get_tx_func (enum os_com_type_t type);


#ifdef __cplusplus
}
#endif

#endif
