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

#ifndef _DRIVERS_ETHDRV_H_
#define _DRIVERS_ETHDRV_H_

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <aos/aos.h>

#include <devices/device.h>

#define MAC_SPEED_10M           1  ///< 10 Mbps link speed
#define MAC_SPEED_100M          2  ///< 100 Mbps link speed
#define MAC_SPEED_1G            3  ///< 1 Gpbs link speed

#define MAC_DUPLEX_HALF         1 ///< Half duplex link
#define MAC_DUPLEX_FULL         2 ///< Full duplex link

typedef struct enc28j60_pin {
    int enc28j60_spi_idx;
    int enc28j60_spi_rst;
    int enc28j60_spi_cs;
    int enc28j60_spi_interrupt;
} enc28j60_pin_t;

typedef struct eth_config {
    int speed;
    int duplex;
    int loopback;
    uint8_t mac[6];
    void *net_pin;
} eth_config_t;

typedef struct eth_driver {
    /*common*/
    int (*mac_control)(dev_t *dev, eth_config_t *config);
    int (*set_packet_filter)(dev_t *dev, int type);
    int (*start)(dev_t *dev);
    int (*restart)(dev_t *dev);
    int (*ping)(dev_t *dev, int type, char *remote_ip);
    int (*ifconfig)(dev_t *dev);
} eth_driver_t;

#endif
