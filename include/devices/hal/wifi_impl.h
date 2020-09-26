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

#ifndef HAL_WIFI_IMPL_H
#define HAL_WIFI_IMPL_H


#include <aos/aos.h>
#include <devices/device.h>
#include <devices/wifi.h>

//30个HAL接口
typedef struct wifi_driver {

    /** common APIs */
    int (*init)(dev_t *dev);
    int (*deinit)(dev_t *dev);
    int (*reset)(dev_t *dev);
    int (*set_mode)(dev_t *dev, wifi_mode_t mode);
    int (*get_mode)(dev_t *dev, wifi_mode_t *mode);
    int (*install_event_cb)(dev_t *dev, wifi_event_func *evt_cb);

    /** conf APIs */
    int (*set_protocol)(dev_t *dev, uint8_t protocol_bitmap); //11bgn
    int (*get_protocol)(dev_t *dev, uint8_t *protocol_bitmap);
    int (*set_country)(dev_t *dev, wifi_country_t country);
    int (*get_country)(dev_t *dev, wifi_country_t *country);
    int (*set_mac_addr)(dev_t *dev, const uint8_t *mac);
    int (*get_mac_addr)(dev_t *dev, uint8_t *mac);
    int (*set_auto_reconnect)(dev_t *dev, bool en);
    int (*get_auto_reconnect)(dev_t *dev, bool *en);
    int (*set_ps)(dev_t *dev, wifi_ps_type_t type); //ps on/pff
    int (*get_ps)(dev_t *dev, wifi_ps_type_t *type);
    int (*power_on)(dev_t *dev); //the wifi module power on/off
    int (*power_off)(dev_t *dev); 

    /** connection APIs */
    int (*start_scan)(dev_t *dev, wifi_scan_config_t *config, bool block);
    int (*start)(dev_t *dev, wifi_config_t * config); //start ap or sta
    int (*stop)(dev_t *dev);//stop ap or sta
    int (*sta_get_link_status)(dev_t *dev, wifi_ap_record_t *ap_info);
    int (*ap_get_sta_list)(dev_t *dev, wifi_sta_list_t *sta);


    /** promiscuous APIs */
    int (*start_monitor)(dev_t *dev, wifi_promiscuous_cb_t cb);
    int (*stop_monitor)(dev_t *dev);
    int (*send_80211_raw_frame)(dev_t *dev, void *buffer, uint16_t len);
    int (*set_channel)(dev_t *dev, uint8_t primary, wifi_second_chan_t second);
    int (*get_channel)(dev_t *dev, uint8_t *primary, wifi_second_chan_t *second);


    /* esp8266 related API */
    int (*set_smartcfg)(dev_t *dev, int enable);

} wifi_driver_t;

#endif


