#include <yoc_config.h>

//#include <devices/devicelist.h>
//#include <devices/device.h>
//#include <devices/driver.h>
//#include <devices/led.h>

#include "app_init.h"

extern int hci_driver_init(char *name);

#define CONFIG_BT_HCI_NAME "hci_ch6121"
extern int hci_driver_ch6121_register(int idx);

void led_set_status(int status)
{
    return;
}

void board_ble_init(void)
{
    hci_driver_ch6121_register(0);
    hci_driver_init(CONFIG_BT_HCI_NAME);
}

