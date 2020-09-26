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

#ifndef HAL_UART_IMPL_H
#define HAL_UART_IMPL_H

#include <stdint.h>

#include <devices/driver.h>

#define USART_EVENT_READ  (1UL << 0)
#define USART_EVENT_WRITE (1UL << 1)
#define USART_OVERFLOW    (1UL << 2)

/*
 * UART data width
 */
typedef enum {
    DATA_WIDTH_5BIT,
    DATA_WIDTH_6BIT,
    DATA_WIDTH_7BIT,
    DATA_WIDTH_8BIT,
    DATA_WIDTH_9BIT
} hal_uart_data_width_t;

/*
 * UART stop bits
 */
typedef enum {
    STOP_BITS_1 = 0,
    STOP_BITS_2 = 1
} hal_uart_stop_bits_t;

/*
 * UART flow control
 */
typedef enum {
    FLOW_CONTROL_DISABLED,
    FLOW_CONTROL_CTS,
    FLOW_CONTROL_RTS,
    FLOW_CONTROL_CTS_RTS
} hal_uart_flow_control_t;

/*
 * UART parity
 */
typedef enum {
    PARITY_NONE = 0, ///< No Parity (default)
    PARITY_EVEN = 1, ///< Even Parity
    PARITY_ODD  = 2, ///< Odd Parity
} hal_uart_parity_t;
/*
 * UART mode
 */
typedef enum {
    MODE_TX,
    MODE_RX,
    MODE_TX_RX
} hal_uart_mode_t;

typedef struct {
    uint32_t                baud_rate;
    hal_uart_data_width_t   data_width;
    hal_uart_parity_t       parity;
    hal_uart_stop_bits_t    stop_bits;
    hal_uart_flow_control_t flow_control;
    hal_uart_mode_t         mode;
} uart_config_t;

typedef struct uart_init_config {
    int idx;
} uart_init_config_t;


typedef struct uart_driver {
    driver_t drv;
    int (*config)(dev_t *dev, uart_config_t *config);
    int (*set_buffer_size)(dev_t *dev, uint32_t size);
    int (*send)(dev_t *dev, const void *data, uint32_t size);
    int (*recv)(dev_t *dev, void *data, uint32_t size, unsigned int timeout_ms);
    int (*set_type)(dev_t *dev, int type);
    void (*set_event)(dev_t *dev, void (*event)(dev_t *dev, int event_id, void *priv), void *priv);
} uart_driver_t;

#endif