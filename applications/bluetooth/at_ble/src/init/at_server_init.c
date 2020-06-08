#include <devices/uart.h>
#include <yoc/atserver.h>
#include <aos/log.h>
#include <yoc/at_cmd.h>
#include <yoc/eventid.h>
#include "at_ble.h"

#define TAG "at_server_init"
extern const atserver_cmd_t at_cmd[];
extern at_conf_load conf_load;

void at_server_init_baud(utask_t *task, uint32_t baud)
{
    uart_config_t config;
    uart_config_default(&config);
    config.baud_rate = baud;

    if (task == NULL) {
        task = utask_new("at_srv", 2 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
    }

    if (task) {
        atserver_init(task, "uart0", &config);
        atserver_set_output_terminator("");
        atserver_add_command(at_cmd);
    }
}
