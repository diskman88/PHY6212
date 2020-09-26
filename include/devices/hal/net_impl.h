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

#include <devices/driver.h>


typedef struct net_ops {
    int (*get_mac_addr)(dev_t *dev, uint8_t *mac);
    int (*set_mac_addr)(dev_t *dev, const uint8_t *mac);

    int (*set_link_up)(dev_t *dev);
    int (*set_link_down)(dev_t *dev);

    int (*start_dhcp)(dev_t *dev);
    int (*stop_dhcp)(dev_t *dev);
    int (*set_ipaddr)(dev_t *dev, const ip_addr_t *ipaddr, const ip_addr_t *netmask, const ip_addr_t *gw);
    int (*get_ipaddr)(dev_t *dev, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);
    int (*ping)(dev_t *dev, int type, char *remote_ip);

    int (*subscribe)(dev_t *dev, uint32_t event, event_callback_t cb, void *param);
    int (*unsubscribe)(dev_t *dev, uint32_t event, event_callback_t cb, void *param);
} net_ops_t;

typedef struct netdev_driver {
    driver_t drv;
    net_ops_t *net_ops;
    int link_type;
    void *link_ops;
} netdev_driver_t;

#endif
