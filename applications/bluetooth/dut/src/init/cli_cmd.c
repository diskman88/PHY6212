/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <yoc_config.h>

#include <aos/cli.h>

void board_cli_init(utask_t *task)
{
    if (task == NULL) {
        task = utask_new("cli", 8 * 1024, QUEUE_MSG_COUNT, 32);
    }

    if (task == NULL) {
        return;
    }

    cli_service_init(task);

    cli_reg_cmd_help();

    cli_reg_cmd_ps();
    cli_reg_cmd_free();

    cli_reg_cmd_sysinfo();
    cli_reg_cmd_kvtool();
}
