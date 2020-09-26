/*
 * printf.c - the file contains the functions printf.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include<limits.h>
#include "minilibc_stdio.h"
#include <stdlib.h>


__attribute__((weak)) int printf(const char *format,...)
{
  int n;
  va_list arg_ptr;
  va_start(arg_ptr, format);
  n=vprintf(format, arg_ptr);
  va_end(arg_ptr);
  return n;
}

