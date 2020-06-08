/*
 * Copyright (C) 2018 C-SKY Microsystems Co., Ltd. All rights reserved.
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

#ifndef __ALI_COAP_INTERNAL_H__
#define __ALI_COAP_INTERNAL_H__

typedef int (*subscribe_cb_t)(const char *topic, void *payload, int len, void *arg);

int alicoap_usrv_init(const char *server_url_suffix);
int alicoap_usrv_connect(void);
int alicoap_usrv_yield(void);
int alicoap_usrv_send(const char *topic, void *payload, int len);
//int alicoap_usrv_subscribe(const char *topic, subscribe_cb_t cb, void *arg);
int alicoap_usrv_disconnect(void);
void alicoap_usrv_deinit(void);

#endif
