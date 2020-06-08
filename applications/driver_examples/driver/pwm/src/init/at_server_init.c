#include <devices/uart.h>
#include <yoc/atserver.h>
#include <aos/log.h>
#include <yoc/at_cmd.h>
#include <yoc/eventid.h>

#define TAG "at_server_init"
extern const atserver_cmd_t at_cmd[];
extern int32_t at_cmd_event_on(void);

static void at_net_event_hdl(uint32_t event_id, const void *data, void *context)
{
    if (at_cmd_event_on()) {
        atserver_lock();

        if (event_id == EVENT_NETMGR_GOT_IP) {
            atserver_send("+EVENT=%s,%s\r\n", "NET", "LINK_UP");
        } else if (event_id == EVENT_NETMGR_NET_DISCON) {
            atserver_send("+EVENT=%s,%s\r\n", "NET", "LINK_DOWN");
        }

        atserver_unlock();
    }

    return;
}

void at_net_event_sub_init(void)
{
    event_subscribe(EVENT_NETMGR_GOT_IP, at_net_event_hdl, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, at_net_event_hdl, NULL);
}

void at_net_event_unsub_init(void)
{
    event_unsubscribe(EVENT_NETMGR_GOT_IP, at_net_event_hdl, NULL);
    event_unsubscribe(EVENT_NETMGR_NET_DISCON, at_net_event_hdl, NULL);
}

void at_server_init(utask_t *task)
{
    uart_config_t config;
    uart_config_default(&config);

    if (task == NULL) {
        task = utask_new("at_srv", 2 * 1024, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);
    }

    if (task) {
        atserver_init(task, "uart1", &config);
        atserver_set_output_terminator("");
        atserver_add_command(at_cmd);
    }

    at_net_event_sub_init();
    //atserver_cmd_show();
}
