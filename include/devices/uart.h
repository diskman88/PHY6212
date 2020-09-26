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

#ifndef DEVICE_UART_PAI_H
#define DEVICE_UART_PAI_H

#include "hal/uart_impl.h"

enum uart_type_t {
    UART_TYPE_GENERAL,
    UART_TYPE_CONSOLE,
};

#define uart_open(name) device_open(name)
#define uart_open_id(name, id) device_open_id(name, id)
#define uart_close(dev) device_close(dev)

int uart_config(dev_t *dev, uart_config_t *config);
int uart_set_type(dev_t *dev, enum uart_type_t type);
int uart_set_buffer_size(dev_t *dev, uint32_t size);
int uart_send(dev_t *dev, const void *data, uint32_t size);
int uart_recv(dev_t *dev, void *data, uint32_t size, unsigned int timeout_ms);
void uart_set_event(dev_t *dev, void (*event)(dev_t *dev, int event_id, void *priv), void *priv);
void uart_config_default(uart_config_t *config);


#endif
