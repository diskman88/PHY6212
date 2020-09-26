/*
 * vsprintf.c - the file contains the functions vsprintf.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>


int vsprintf(char *dest,const char *format, va_list arg_ptr)
{
  return vsnprintf(dest,(size_t)0x7FFFFFFF,format,arg_ptr);
}
