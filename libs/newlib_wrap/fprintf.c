/*
 * fprintf.c - the file contains the functions fprintf.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include "minilibc_stdio.h"
#include <stdarg.h>
#include <stdlib.h>

int fprintf(FILE *f,const char *format,...) {
  int n;
  va_list arg_ptr;
  va_start(arg_ptr,format);
  n=vfprintf(f,format,arg_ptr);
  va_end(arg_ptr);
  return n;
}
