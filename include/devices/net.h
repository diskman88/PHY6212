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

#ifndef DRIVER_NETDRV_H
#define DRIVER_NETDRV_H

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>

#include <yoc/event.h>

#include "driver.h"
#include "device.h"
#include <sys/socket.h>
#include "hal/net_impl.h"

#define EVENT_NET_GOT_IP        0x100

#define EVENT_WIFI_LINK_DOWN    0x101
#define EVENT_WIFI_LINK_UP      0x102
#define EVENT_WIFI_EXCEPTION    0x103
#define EVENT_WIFI_SMARTCFG     0x105

#define EVENT_ETH_LINK_UP       0x111
#define EVENT_ETH_LINK_DOWN     0x112
#define EVENT_ETH_EXCEPTION     0x113

//nbiot linkup&gotip todo
#define EVENT_NBIOT_LINK_UP     0x122
#define EVENT_NBIOT_LINK_DOWN   0x123

#define EVENT_GPRS_LINK_UP      0x132
#define EVENT_GPRS_LINK_DOWN    0x133

/* EVENT_NETMGR_NET_DISCON(LINK DOWN) REASON */
#define NET_DISCON_REASON_NORMAL         255
#define NET_DISCON_REASON_WIFI_TIMEOUT   1
#define NET_DISCON_REASON_WIFI_PSK_ERR   2
#define NET_DISCON_REASON_WIFI_NOEXIST   3
#define NET_DISCON_REASON_ERROR          4
#define NET_DISCON_REASON_DHCP_ERROR     5

#define EVENT_NETWORK_RESTART (140)


enum {
    NETDEV_TYPE_ETH,
    NETDEV_TYPE_WIFI,
    NETDEV_TYPE_GPRS,
    NETDEV_TYPE_NBIOT,
};


/**
    These APIs define Ethernet level operation
*/
int hal_net_get_mac_addr(dev_t *dev, uint8_t *mac);
int hal_net_set_mac_addr(dev_t *dev, const uint8_t *mac);
int hal_net_set_link_up(dev_t *dev);
int hal_net_set_link_down(dev_t *dev);
int hal_net_start_dhcp(dev_t *dev);
int hal_net_stop_dhcp(dev_t *dev);
int hal_net_set_ipaddr(dev_t *dev, const ip_addr_t *ipaddr, const ip_addr_t *netmask, const ip_addr_t *gw);
int hal_net_get_ipaddr(dev_t *dev, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);
int hal_net_ping(dev_t *dev, int type, char *remote_ip);
int hal_net_subscribe(dev_t *dev, uint32_t event, event_callback_t cb, void *param);
int hal_net_unsubscribe(dev_t *dev, uint32_t event, event_callback_t cb, void *param);


#endif
