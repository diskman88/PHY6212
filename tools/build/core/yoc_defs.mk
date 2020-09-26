##
 # Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #   http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
##

####net####
ifeq ($(CONFIG_NET_DRIVER), esp8266)
YOC_DEFS += -DCONFIG_SAL=1 -DCONFIG_SAL_ESP8266=1
YOC_LIBS += ksal
else ifeq ($(CONFIG_NET_DRIVER), enc28j60)
YOC_DEFS += -DCONFIG_TCPIP=1 -DCONFIG_TCPIP_ENC28J60=1
YOC_LIBS += klwip
else ifeq ($(CONFIG_NET_DRIVER), sim800c)
YOC_DEFS += -DCONFIG_SAL=1 -DCONFIG_SAL_SIM800C=1
YOC_LIBS += ksal
else ifeq ($(CONFIG_NET_DRIVER), wifi6700s)
YOC_DEFS += -DCONFIG_TCPIP=1 -DCONFIG_WIFI6700S=1
YOC_LIBS += klwip wifi6700s
else ifeq ($(CONFIG_NET_DRIVER), rtl8723ds)
YOC_DEFS += -DCONFIG_TCPIP=1 -DCONFIG_WIFI_RTL8723DS=1
YOC_LIBS += klwip rtl8723ds
endif

####iot####
ifeq ($(CONFIG_IOT_LIBS),iot_alicoap_psk)
YOC_DEFS += -DCONFIG_CLOUDIO_ALICOAP=1 -DCONFIG_ALICOAP_PSK=1
YOC_LIBS += iot_aliot

else ifeq ($(CONFIG_IOT_LIBS),iot_alicoap_dtls)
YOC_DEFS += -DCONFIG_CLOUDIO_ALICOAP=1 -DCONFIG_ALICOAP_DTLS=1
YOC_LIBS += iot_aliot

else ifeq ($(CONFIG_IOT_LIBS),iot_alimqtt_tls)
YOC_DEFS += -DCONFIG_CLOUDIO_ALIMQTT=1 -DCONFIG_ALIMQTT_TLS=1
YOC_LIBS += iot_aliot

else ifeq ($(word 1, $(CONFIG_IOT_LIBS)),iot_onenet)
YOC_DEFS += -DCONFIG_CLOUDIO_ONENET=1

else ifeq ($(word 1, $(CONFIG_IOT_LIBS)),iot_lwm2m)
YOC_DEFS += -DCONFIG_CLOUDIO_OCEANCON=1

endif

####other####
ifneq ($(findstring fota,$(CONFIG_LIBS)),)
YOC_DEFS += -DCONFIG_FOTA=1
endif

ifneq ($(findstring cli,$(CONFIG_LIBS)),)
YOC_DEFS += -DCONFIG_CLI=1
endif

####User define####
YOC_DEFS += $(CONFIG_USER_DEFINE)
