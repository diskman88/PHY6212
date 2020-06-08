#include "os_com_service.h"
#include "os_com_port.h"
#include <stdio.h>
#include <drv_usart.h>
#include <pin.h>
#include <soc.h>


static os_com_service_tx_func_t  g_os_com_ser_txs[] =
{
#if OS_COM_RS232
  os_com_rs232_transmit,
#else
  NULL,
#endif
#if OS_COM_JTAG
  os_com_jtag_transmit,
#else
  NULL,
#endif
#if OS_COM_USB
  os_com_usb_transmit,
#else
  NULL,
#endif
#if OS_COM_TCPIP
  os_com_tcpip_transmit,
#else
  NULL,
#endif
};


os_com_service_tx_func_t
os_com_get_tx_func (enum os_com_type_t type)
{
  return g_os_com_ser_txs[type];
}

#if OS_COM_RS232
static usart_handle_t phandle;
static char rdata = 0;
void os_com_event_cb(int32_t idx, usart_event_e event)
{
    if (event == USART_EVENT_RECEIVE_COMPLETE)
        {
            os_com_service_rx_data (OS_COM_TYPE_RS232, (uint8_t *)&rdata, 1);
            csi_usart_receive(phandle, &rdata, 1);
        }
}

int32_t os_com_rs232_init (void)
{
    int32_t ret;
    phandle = csi_usart_initialize(COMM_IDX, os_com_event_cb);
    if (phandle == NULL) {
        printf("csi_usart_initialize error\n");
        return 1;
    }

    ret = csi_usart_config(phandle, OS_COM_RS232_BAUDRATE,
                           USART_MODE_ASYNCHRONOUS, OS_COM_RS232_PARITY,
                           OS_COM_RS232_STOP, OS_COM_RS232_BITS);

    if (ret < 0) {
        printf("csi_usart_config error %x\n", ret);
        return -1;
    }

    csi_usart_receive(phandle, &rdata, 1);

    return ret;
}


int32_t os_com_rs232_transmit(uint8_t*pdata, uint32_t nbytes)
{
    int time_out = 0x7ffff;
    usart_status_t status;

    csi_usart_send(phandle, pdata, nbytes);

    while (time_out) {
        time_out--;
        status = csi_usart_get_status(phandle);

        if (!status.tx_busy) {
            break;
        }
    }
    if (0 == time_out) {
        return -1;
    }

    return 0;
}
#endif
