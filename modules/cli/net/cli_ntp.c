/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc_config.h>
#if defined(CONFIG_CLI) && ( defined(CONFIG_TCPIP) || defined(CONFIG_SAL))
#include <aos/cli.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern int ntp_sync_time(char *server);
static void cmd_ntp_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int item_count = argc;

    if (item_count >= 2) {
        if (strcmp(argv[1], "ntp") == 0) {
            if (item_count == 2) {
                ntp_sync_time(NULL);
            } else {
                ntp_sync_time(argv[2]);
            }
        }
        else {
            printf("\tntp cmd:date ntp server\n");    
        }
    } else {
        time_t t = time(NULL);
        printf("\tnow:%s\n", ctime(&t));
        printf("\tntp cmd:date ntp server\n");
    }
}

void cli_reg_cmd_ntp(void)
{
    static const struct cli_command cmd_info = {
        "date",
        "date command.",
        cmd_ntp_func
    };

    aos_cli_register_command(&cmd_info);
}

#endif /*defined(CONFIG_CLI) && ( defined(CONFIG_TCPIP)) */
