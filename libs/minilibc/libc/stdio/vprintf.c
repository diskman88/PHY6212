#include <stdarg.h>
#include <devices/uart.h>

#include "serf/minilibc_stdio.h"
#include <devices/uart.h>
#include <csi_core.h>

extern dev_t *g_console_handle;
extern void os_critical_enter();
extern void os_critical_exit();

static int __stdio_outs(const char *s,size_t len) {
    int i;

    for(i = 0; i < len; i++) {
        fputc(*(s+i), stdout);
    }
    return 1;
}

int vprintf(const char *format, va_list ap)
{
  struct arg_printf _ap = { 0, (int(*)(void*,size_t,void*)) __stdio_outs };
  return yoc__v_printf(&_ap,format,ap);
}

