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

#include <stdio.h>
#include <string.h>
//#include <unistd.h>

#include <devices/uart.h>

#define UART_DRIVER(dev)  ((uart_driver_t*)(dev->drv))
#define UART_VAILD(dev) do { \
    if (device_valid(dev, "uart") != 0) \
        return -1; \
} while(0)

void uart_config_default(uart_config_t *config)
{
    config->baud_rate = 115200;
    config->data_width = DATA_WIDTH_8BIT;
    config->parity = PARITY_NONE;
    config->stop_bits = STOP_BITS_1;
    config->flow_control = FLOW_CONTROL_DISABLED;
    config->mode = MODE_TX_RX;
}

int uart_config(dev_t *dev, uart_config_t *config)
{
    int ret;

    UART_VAILD(dev);

    device_lock(dev);
    ret = UART_DRIVER(dev)->config(dev, config);
    device_unlock(dev);

    return ret;
}

int uart_set_type(dev_t *dev, enum uart_type_t type)
{
    int ret;

    UART_VAILD(dev);

    device_lock(dev);
    ret = UART_DRIVER(dev)->set_type(dev, type);
    device_unlock(dev);

    return ret;
}

int uart_set_buffer_size(dev_t *dev, uint32_t size)
{
    int ret;

    UART_VAILD(dev);

    device_lock(dev);
    ret = UART_DRIVER(dev)->set_buffer_size(dev, size);
    device_unlock(dev);

    return ret;
}

int uart_send(dev_t *dev, const void *data, uint32_t size)
{
    if (size == 0 || NULL == data) {
        return -EINVAL;
    }

    int ret;

    UART_VAILD(dev);

    device_lock(dev);
    ret = UART_DRIVER(dev)->send(dev, data, size);
    device_unlock(dev);

    return ret;
}

int uart_recv(dev_t *dev, void *data, uint32_t size, unsigned int timeout_ms)
{
    int ret;

    UART_VAILD(dev);

    device_lock(dev);
    ret = UART_DRIVER(dev)->recv(dev, data, size, timeout_ms);
    device_unlock(dev);

    return ret;
}

void uart_set_event(dev_t *dev, void (*event)(dev_t *dev, int event_id, void *priv), void *priv)
{
    aos_check_param(dev);
    if (device_valid(dev, "uart") != 0)
        return;

    device_lock(dev);
    UART_DRIVER(dev)->set_event(dev, event, priv);
    device_unlock(dev);
}

