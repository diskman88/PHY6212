/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
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
#ifndef __DUT_AT_CMD_H_
#define __DUT_AT_CMD_H_

int dut_cmd_rx_mode(int argc, char *argv[]);

int dut_cmd_ftest(int argc, char *argv[]);

int dut_cmd_sleep(int argc, char *argv[]);

int dut_cmd_ireboot(int argc, char *argv[]);

int dut_cmd_opt_mac(int argc, char *argv[]);
int fdut_cmd_opt_mac(int argc, char *argv[]);


int dut_cmd_freq_off(int argc, char *argv[]);
int fdut_cmd_freq_off(int argc, char *argv[]);


void dut_at_cmd_init(void);

#endif
