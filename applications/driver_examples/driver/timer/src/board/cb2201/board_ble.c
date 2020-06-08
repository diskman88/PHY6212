#include <yoc_config.h>

#include <devices/devicelist.h>
#include <devices/device.h>
#include <devices/driver.h>
#include <devices/led.h>
#include "pin.h"
#include "app_init.h"

//#define CONFIG_BT_HCI_RAFAEL

extern int hci_driver_init(char *name);

#ifdef CONFIG_BT_HCI_RAFAEL
#define CONFIG_BT_HCI_NAME "hci_rafael"
extern int hci_driver_rafael_register(int idx);
#else
#define CONFIG_BT_HCI_NAME "hci_h4"
extern int hci_driver_h4_register(int idx);
#endif

static led_pin_config_t led_config = {PA12, PA13, -1, 1};
static dev_t *led = NULL;

void led_set_status(int status)
{
    switch (status) {
        case ON:
            led_control(led, COLOR_WHITE, -1, -1);
            break;

        case OFF:
            led_control(led, COLOR_BLACK, -1, -1);
            break;

        case BLINK_FAST:
            led_control(led, COLOR_WHITE, 200, 200);
            break;

        case BLINK_SLOW:
            led_control(led, COLOR_WHITE, 1000, 1000);
            break;

        default:
            led_control(led, COLOR_BLACK, -1, -1);
            break;
    }
}

void board_ble_init(void)
{
    led_rgb_register(&led_config, 0);
    led = led_open("ledrgb0");

#ifdef CONFIG_BT_HCI_RAFAEL
    hci_driver_rafael_register(0);
#else
    hci_driver_h4_register(0);
#endif

    hci_driver_init(CONFIG_BT_HCI_NAME);
}
