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

#ifndef DEVICE_AT_UART_H
#define DEVICE_AT_UART_H

#include <stdint.h>

int32_t at_uart_init(void);
int32_t at_uart_deinit(void);

void at_uart_send_str(const char *fmt, ...);
void at_uart_send_data(uint8_t *data, uint32_t len);

void at_uart_send_evt_str(const char *name, const char *para);
void at_uart_send_evt_str_errid(const char *name, const char *para, int errid);
void at_uart_send_evt_int(const char *name, uint32_t val);
void at_uart_send_evt_recv(const char *name, uint16_t id, void *payload, uint16_t tot_len);

#endif
