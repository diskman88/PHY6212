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

#ifndef _DEVICE_GPRS_MODULE_H
#define _DEVICE_GPRS_MODULE_H

/* Define platform endianness */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif /* BYTE_ORDER */

#define SIM800_MAX_LINK_NUM  5

typedef struct {
    int rssi;
    int ber;
} sig_qual_resp_t;

typedef struct {
    int n;
    int stat;
} regs_stat_resp_t;

/* Change to include data slink for each link id respectively. <TODO> */

typedef enum {
    AT_TYPE_NULL = 0,
    AT_TYPE_TCP_SERVER,
    AT_TYPE_TCP_CLIENT,
    AT_TYPE_UDP_UNICAST,
    AT_TYPE_MAX
} at_conn_e;

typedef enum {
    SAL_INIT_CMD_RETRY = 21,
    SAL_INIT_CMD_SIMCARD,
    SAL_INIT_CMD_QUELITY,
    SAL_INIT_CMD_GPRS_CLOSE,
    SAL_INIT_CMD_PDPC,
    SAL_INIT_CMD_GPRSMODE,
    SAL_INIT_CMD_CREG,
    SAL_INIT_CMD_GATT,
    SAL_INIT_CMD_MUX,
    SAL_INIT_CMD_CSTT,
    SAL_INIT_CMD_CIICR,
    SAL_INIT_CMD_END,
} enum_init_cmd_e;

#define SAL_MODULE_RESET  0x55

typedef enum {
    GPRS_STATUS_LINK_DISCONNECTED = 0,
    GPRS_STATUS_LINK_CONNECTED,
} gprs_status_link_t;

typedef enum {
    CONNECT_MODE_CSD = 0,    /**< CSD mode */
    CONNECT_MODE_GPRS,       /**< GPRS mode */
} gprs_mode_t;

typedef struct gprs_driver {
    /*common*/
    int (*set_mode)(dev_t *dev, gprs_mode_t mode);
    int (*get_mode)(dev_t *dev, gprs_mode_t *mode);
    int (*reset)(dev_t *dev);
    int (*start)(dev_t *dev);
    int (*stop)(dev_t *dev);
    /*configuration*/
    int (*set_if_config)(dev_t *dev, uint32_t baud, uint8_t flow_control);
    /*connection*/
    int (*module_init_check)(dev_t *dev);
    int (*connect_to_gprs)(dev_t *dev);
    int (*disconnect_from_gprs)(dev_t *dev);
    int (*get_link_status)(dev_t *dev, gprs_status_link_t *link_status);
    int (*get_ipaddr)(dev_t *dev, char ip[16]);
    int (*get_csq)(dev_t *dev, int *csq);
    int (*get_simcard_info)(dev_t *dev, char ccid[21], int *insert);
} gprs_driver_t;


#endif /*_WIFI_MODULE*/
