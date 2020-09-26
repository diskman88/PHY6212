#include <stdio.h>
#include <aos/log.h>
#include <aos/kernel.h>
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

#include <devices/uart.h>

#include "ringbuffer.h"

#include "drv_usart.h"

#include <yoc/lpm.h>

#define UART_RB_SIZE 2048
#define UART_NUM     1

#define TAG "uart_drv"

#define EVENT_WRITE 0x0F0F0000
#define EVENT_READ 0x00000F0F

typedef struct uartdev_priv {
    int            flowctrl;
    usart_handle_t handle;
    char          *recv_buf;
    ringbuffer_t   read_buffer;
    aos_event_t    event_write_read;
    int            event_state;
    void (*write_event)(dev_t *dev, int event_id, void *priv);
    void *priv;
    void *init_cfg;
    int   type;
} uartdev_priv_t;

typedef struct {
    dev_t          device;
    uartdev_priv_t *priv;
} uart_dev_t;

#define uart(dev) (((uart_dev_t *)dev)->priv)

static char recv_buf[UART_NUM][UART_RB_SIZE];
static uartdev_priv_t uart_priv[UART_NUM];

static dev_t *uart_csky_init(driver_t *drv, void *config, int id)
{
    uart_dev_t *uart = (uart_dev_t *)device_new(drv, sizeof(uart_dev_t), id);
    uart->priv = &uart_priv[id];
    uart(uart)->init_cfg = config;
    return (dev_t *)uart;
}

#define uart_csky_uninit device_free

static void usart_csky_event_cb_fun(int32_t idx, usart_event_e event)
{
    static uart_dev_t *uart_idx[UART_NUM];
    uart_dev_t *uart;

    if (uart_idx[idx] == NULL) {
        uart_idx[idx] = (uart_dev_t *)device_find("uart", idx);
    }

    uart = uart_idx[idx];

    switch (event) {
        case USART_EVENT_SEND_COMPLETE:
            if ((uart(uart)->event_state & USART_EVENT_WRITE) == 0) {
                uart(uart)->event_state |= USART_EVENT_WRITE;

                if (uart(uart)->write_event) {
                    uart(uart)->write_event((dev_t *)uart, USART_EVENT_WRITE, uart(uart)->priv);
                }
            }

            if (aos_event_set(&uart(uart)->event_write_read, EVENT_WRITE, AOS_EVENT_OR) != 0) {
            }

            break;

        case USART_EVENT_RECEIVED: {
            int32_t ret;
            uint8_t recv_data[24] = {0};
#if 0

            /* flow ctrl */
            if (usart_dev->flowctrl) {
                ret = ringbuffer_available_write_space(usart_dev->read_buffer);

                if (ret <= 64) {
                    csi_usart_interrupt_on_off(handle, USART_INTR_READ, 0);
                    break;
                }
            }

#endif

            if (uart(uart)->recv_buf != NULL) {
                ret = csi_usart_receive_query(uart(uart)->handle, recv_data, sizeof(recv_data));

                if (ret > 0) {
                    if (ringbuffer_write(&uart(uart)->read_buffer, (uint8_t *)recv_data, ret) != ret) {
                        if (uart(uart)->write_event) {
                            uart(uart)->write_event((dev_t *)uart, USART_OVERFLOW, uart(uart)->priv);
                        }

                        break;
                    }
                }
            }

            if ((uart(uart)->event_state & USART_EVENT_READ) == 0) {
                if (uart(uart)->write_event) {
                    uart(uart)->event_state |= USART_EVENT_READ;
                    uart(uart)->write_event((dev_t *)uart, USART_EVENT_READ, uart(uart)->priv);
                }
            }

            aos_event_set(&uart(uart)->event_write_read, EVENT_READ, AOS_EVENT_OR);
            break;
        };

        case USART_EVENT_RX_OVERFLOW: {
            /* lost some data, clean hw buffer and ringbuffer */
            char data[16];
            csi_usart_receive_query(uart(uart)->handle, data, 16);

            if (uart(uart)->recv_buf != NULL) {
                ringbuffer_clear(&uart(uart)->read_buffer);
            }

            uart(uart)->event_state &= ~USART_EVENT_READ;
            break;
        }

        case USART_EVENT_RX_FRAMING_ERROR:
        default:
            // LOGW(TAG, "uart%d event %d", idx, event);
            break;
    }
}

static int uart_csky_open(dev_t *dev)
{
    if (aos_event_new(&uart(dev)->event_write_read, 0) != 0) {
        goto error;
    }

    uart(dev)->recv_buf = recv_buf[dev->id];

    ringbuffer_create(&uart(dev)->read_buffer, uart(dev)->recv_buf, UART_RB_SIZE);

    uart(dev)->handle = csi_usart_initialize(dev->id, usart_csky_event_cb_fun);

    if (uart(dev)->handle != NULL) {
        return 0;
    }

error:
    return -1;
}

static int uart_csky_close(dev_t *dev)
{
    csi_usart_uninitialize(uart(dev)->handle);
    aos_event_free(&uart(dev)->event_write_read);

    return 0;
}

static int uart_csky_config(dev_t *dev, uart_config_t *config)
{
    int32_t ret = csi_usart_config(uart(dev)->handle, config->baud_rate, USART_MODE_ASYNCHRONOUS,
                                   config->parity, config->stop_bits, config->data_width);

    if (ret < 0) {
        return -EIO;
    }

    ret = csi_usart_config_flowctrl(uart(dev)->handle, config->flow_control);

    if (ret == 0) {
        uart(dev)->flowctrl = 1;
    }

    return 0;
}

static int uart_csky_set_type(dev_t *dev, int type)
{
    uart(dev)->type = type;

    return 0;
}

static int uart_csky_set_buffer_size(dev_t *dev, uint32_t size)
{
    return -1;
}

static int uart_csky_send(dev_t *dev, const void *data, uint32_t size)
{
    if (uart(dev)->type == UART_TYPE_CONSOLE) {
        int i;

        for (i = 0; i < size; i++) {
            //dont depend interrupt
            csi_usart_putchar(uart(dev)->handle, *((uint8_t *)data + i));
        }
    } else {
        unsigned int actl_flags = 0;
        csi_usart_send(uart(dev)->handle, data, size);
        aos_event_get(&uart(dev)->event_write_read, EVENT_WRITE, AOS_EVENT_OR_CLEAR, &actl_flags,
                      AOS_WAIT_FOREVER);
    }

    return 0;
}

static int uart_csky_recv(dev_t *dev, void *data, uint32_t size, unsigned int timeout_ms)
{
    unsigned int actl_flags;
    int          ret = 0;
    long long    time_enter, used_time;
    void        *temp_buf   = data;
    uint32_t     temp_count = size;
    time_enter              = aos_now_ms();

    while (1) {
        if (uart(dev)->recv_buf != NULL) {
            ret = ringbuffer_read(&uart(dev)->read_buffer, (uint8_t *)temp_buf, temp_count);
        } else {
            ret = csi_usart_receive_query(&uart(dev)->handle, (uint8_t *)temp_buf, temp_count);
        }

        temp_count = temp_count - ret;
        temp_buf   = (uint8_t *)temp_buf + ret;
        used_time  = aos_now_ms() - time_enter;

        if (timeout_ms <= used_time || temp_count == 0) {
            break;
        }

        if (aos_event_get(&uart(dev)->event_write_read, EVENT_READ, AOS_EVENT_OR_CLEAR, &actl_flags,
                          timeout_ms - used_time) == -1) {
            break;
        }
    }

    uart(dev)->event_state &= ~USART_EVENT_READ;
    return size - temp_count;
}

static void uart_csky_event(dev_t *dev, void (*event)(dev_t *dev, int event_id, void *priv),
                            void *priv)
{
    uart(dev)->priv        = priv;
    uart(dev)->write_event = event;
}

enum {
    EVENT_SEND_COMPLETE     = 0,  ///< Send completed; however USART may still transmit data
    EVENT_RECEIVE_COMPLETE  = 1,  ///< Receive completed
    EVENT_TRANSFER_COMPLETE = 2,  ///< Transfer completed
    EVENT_TX_COMPLETE       = 3,  ///< Transmit completed (optional)
    EVENT_TX_UNDERFLOW      = 4,  ///< Transmit data not available (Synchronous Slave)
    EVENT_RX_OVERFLOW       = 5,  ///< Receive data overflow
    EVENT_RX_TIMEOUT        = 6,  ///< Receive character timeout (optional)
    EVENT_RX_BREAK          = 7,  ///< Break detected on receive
    EVENT_RX_FRAMING_ERROR  = 8,  ///< Framing error detected on receive
    EVENT_RX_PARITY_ERROR   = 9,  ///< Parity error detected on receive
    EVENT_CTS               = 10, ///< CTS state changed (optional)
    EVENT_DSR               = 11, ///< DSR state changed (optional)
    EVENT_DCD               = 12, ///< DCD state changed (optional)
    EVENT_RI                = 13, ///< RI  state changed (optional)
    EVENT_RECEIVED          = 14, ///< Data Received, only in usart fifo, call receive()/transfer() get the data
};

static uart_driver_t uart_driver = {
    .drv = {
        .name   = "uart",
        .init   = uart_csky_init,
        .uninit = uart_csky_uninit,
        .open   = uart_csky_open,
        .close  = uart_csky_close,
    },
    .config          = uart_csky_config,
    .set_type        = uart_csky_set_type,
    .set_buffer_size = uart_csky_set_buffer_size,
    .send            = uart_csky_send,
    .recv            = uart_csky_recv,
    .set_event       = uart_csky_event,
};

void uart_csky_register(int idx)
{
    driver_register(&uart_driver.drv, NULL, idx);
}

usart_handle_t dev_get_handler(dev_t *dev)
{
    uart_dev_t *uart = (uart_dev_t *)dev;

    return uart(uart)->handle;
}
