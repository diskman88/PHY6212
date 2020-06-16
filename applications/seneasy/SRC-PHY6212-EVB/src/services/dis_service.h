#ifndef _DIS_SERVICE_H
#define _DIS_SERVICE_H

#include <yoc_config.h>
#include <aos/log.h>
#include <aos/kernel.h>
#include <aos/ble.h>
#include <app_init.h>
#include <yoc/dis.h>
#include <yoc/cli.h>
#include <aos/cli.h>
#include <devices/devicelist.h>
#include <stdint.h>
#include <stdbool.h>
#include <aos/log.h>

#define MANUFACTURER_NAME "PINGTOUGE"
#define MODEL_NUMBER "MODE_KEYBOARD"
#define SERIAL_NUMBER "00000001"
#define HW_REV "0.0.1"
#define FW_REV "0.0.2"
#define SW_REV "0.0.3"

int dis_service_init();

#endif
