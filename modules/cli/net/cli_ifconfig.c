/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <yoc_config.h>

#include <aos/cli.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <yoc/netmgr.h>

#include <devices/driver.h>
#include <devices/ethernet.h>
#include <devices/wifi.h>
#include <devices/net.h>


#if defined(CONFIG_TCPIP) || defined(CONFIG_TCPIP_NBIOT)

#define USAGE_INFO    "\n\tifconfig ip/drip/mask/dns ipaddr\n\tifconfig ipv6 ipaddr\n\tifconfig dhcpstart/dhcpstop\n"

#elif defined(CONFIG_SAL)

#define USAGE_INFO    "\n\tifconfig status/ip\n\tOnly wifi support:\n\tifconfig ap ssid psw\n\tifconfig apoff\n\tifconfig uartcfg 115200 8 1 0 3 1\n"

#else

#define USAGE_INFO    "\n\tcmd is useless\n"

#endif

static void cmd_ifconfig_eth(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;

    dev_t *eth_dev = device_find("eth", 0);

    if (eth_dev != NULL) {

        netmgr_hdl_t hdl_eth = netmgr_get_handle("eth");

        if (item_count == 2) {
#if LWIP_DHCP
            if (strcmp(argv[1], "dhcpstart") == 0) {
                printf("start dhcp\n");
                netmgr_ipconfig(hdl_eth, 1, NULL, NULL, NULL);
            } else if (strcmp(argv[1], "dhcpstop") == 0) {
                printf("stop dhcp\n");
                netmgr_ipconfig(hdl_eth, 0, NULL, NULL, NULL);
            }
#endif

            return;
        } else {
            netmgr_get_info(hdl_eth);
            return;
        }
    }
    printf(USAGE_INFO);
}

static void cmd_ifconfig_wifi(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;
    dev_t *wifi_dev = device_find("wifi", 0);

    if (wifi_dev != NULL) {
        netmgr_hdl_t hdl = netmgr_get_handle("wifi");

        if (item_count == 2) {
            if (strcmp(argv[1], "apoff") == 0) {
                netmgr_stop(hdl);
                printf("apoff\n");
                return;
            }
        } else if (item_count == 4) {
            if (strcmp(argv[1], "ap") == 0) {
                printf("apconfig ssid:%s, psw:%s\n", argv[2], argv[3]);

                netmgr_config_wifi(hdl, argv[2], strlen(argv[2]), argv[3], strlen(argv[3]));
                netmgr_start(hdl);
            }

            return;
        } else {
            netmgr_get_info(hdl);

            return;
        }
    }
    printf(USAGE_INFO);
}

static void cmd_ifconfig_gprs(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;
    dev_t *gprs_dev = device_find("gprs", 0);

    if (gprs_dev != NULL) {

        netmgr_hdl_t hdl = netmgr_get_handle("gprs");

        if (item_count == 1) {

            netmgr_get_info(hdl);

            return;
        }
    }
    printf(USAGE_INFO);
}

static void cmd_ifconfig_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;

    dev_t *eth_dev = device_find("eth", 0);

    if (eth_dev != NULL) {

        netmgr_hdl_t hdl_eth = netmgr_get_handle("eth");

        if (item_count == 2) {
#if LWIP_DHCP
            if (strcmp(argv[1], "dhcpstart") == 0) {
                printf("start dhcp\n");
                netmgr_ipconfig(hdl_eth, 1, NULL, NULL, NULL);
            } else if (strcmp(argv[1], "dhcpstop") == 0) {
                printf("stop dhcp\n");
                netmgr_ipconfig(hdl_eth, 0, NULL, NULL, NULL);
            }
#endif

            return;
        } else {
            netmgr_get_info(hdl_eth);
            return;
        }
    }

    dev_t *wifi_dev = device_find("wifi", 0);

    if (wifi_dev != NULL) {
        netmgr_hdl_t hdl = netmgr_get_handle("wifi");

        if (item_count == 2) {
            if (strcmp(argv[1], "apoff") == 0) {
                netmgr_stop(hdl);
                printf("apoff\n");
                return;
            }
        } else if (item_count == 4) {
            if (strcmp(argv[1], "ap") == 0) {
                printf("apconfig ssid:%s, psw:%s\n", argv[2], argv[3]);

                netmgr_config_wifi(hdl, argv[2], strlen(argv[2]), argv[3], strlen(argv[3]));
                netmgr_start(hdl);
            }

            return;
        } else {
            netmgr_get_info(hdl);

            return;
        }
    }

    dev_t *gprs_dev = device_find("gprs", 0);

    if (gprs_dev != NULL) {

        netmgr_hdl_t hdl = netmgr_get_handle("gprs");

        if (item_count == 1) {

            netmgr_get_info(hdl);

            return;
        }
    }

    printf(USAGE_INFO);
}

void cli_reg_cmd_ifconfig(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_func,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ifconfig_eth(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_eth,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ifconfig_wifi(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_wifi,
    };

    aos_cli_register_command(&cmd_info);
}

void cli_reg_cmd_ifconfig_gprs(void)
{
    static const struct cli_command cmd_info = {
        "ifconfig",
        "network config",
        cmd_ifconfig_gprs,
    };

    aos_cli_register_command(&cmd_info);
}