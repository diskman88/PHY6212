/*
 * vprintf.c - the file contains the functions vprintf.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include <stdarg.h>
#include "minilibc_stdio.h"
FILE *stdin, *stdout, *stderr;


extern void os_critical_enter();
extern void os_critical_exit();
int __stdio_outs(const char *s,size_t len) __attribute__((weak));
int __stdio_outs(const char *s,size_t len) {

     int i;
     os_critical_enter();
     for(i = 0; i < len; i++)
   	fputc(*(s+i), stdout);
     os_critical_exit();

     return 1;
}
int vprintf(const char *format, va_list ap)
{
  struct arg_printf _ap = { 0, (int(*)(void*,size_t,void*)) __stdio_outs };
  return __v_printf(&_ap,format,ap);
}

