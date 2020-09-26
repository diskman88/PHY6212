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

#ifndef HAL_NETMGR_IMPL_H
#define HAL_NETMGR_IMPL_H

#include <stdint.h>
#include <sys/socket.h>

#include <devices/driver.h>

typedef struct netif_driver {
    driver_t drv;
    void (*get_mac_addr)(netif_driver_t *netif, uint8_t *mac);
    void (*set_mac_addr)(netif_driver_t *netif, const uint8_t *mac);

    void (*set_link_up)(netif_driver_t *netif);
    void (*set_link_down)(netif_driver_t *netif);

    void (*set_ipaddr)(netif_driver_t *netif, const ip4_addr_t *ipaddr);
    void (*set_netmask)(netif_driver_t *netif, const ip4_addr_t *netmask);
    void (*set_gw)(netif_driver_t *netif, const ip4_addr_t *gw);
} netif_driver_t;


typedef struct netif_at_driver {
    netif_driver_t netif;
    sal_op_t sal_op;

} netif_at_driver_t;

#endif