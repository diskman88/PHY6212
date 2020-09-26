/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <yoc_config.h>

#include <aos/cli.h>
#include <aos/kernel.h>

#include <yoc/lpm.h>

#define HELP_INFO "Usage: lpm policy [0|1|2]\n\t lpm ls_th <ms>\n\t lpm ds_th <ms>\n"

void cmd_lpm_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    if (argc < 3) {
        printf("%s\n", HELP_INFO);
        return;
    }

    if (0 == strcmp(argv[1], "policy")){
        if (argc == 3) {
            aos_kv_setint(KV_LPM_POLICY, atoi(argv[2]));
        }else{
            printf(HELP_INFO);
        }
    } else if (0 == strcmp(argv[1], "ls_th")){
        if (argc == 3) {
            aos_kv_setint(KV_LPM_LS_THRES, atoi(argv[2]));
        }else{
            printf(HELP_INFO);
        }
    } else if (0 == strcmp(argv[1], "ds_th")){
        if (argc == 3) {
            aos_kv_setint(KV_LPM_DS_THRES, atoi(argv[2]));
        }else{
            printf(HELP_INFO);
        }
    } else {
        printf(HELP_INFO);
    }
}

void cli_reg_cmd_lpm(void)
{
    static const struct cli_command cmd_info = 
    {
        "lpm",
        "lpm command",
        cmd_lpm_func
    };

    aos_cli_register_command(&cmd_info);
}

