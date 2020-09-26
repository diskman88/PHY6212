#ifndef __YOC_CONFIG_H__
#define __YOC_CONFIG_H__

#define CONSOLE_ID 0

#ifdef CONFIG_SAL
#define CONFIG_SAL_ESP8266    1
#endif

#ifdef CONFIG_TCPIP
#define CONFIG_TCPIP_ENC28J60   1
#endif

#define CONFIG_DEBUG    1
#endif
