/*
 * snprintf.c - the file contains the functions snprintf.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include "minilibc_stdio.h"

int snprintf(char *str, size_t size, const char *format, ...)
{
	int n;
	va_list arg_ptr;
	va_start(arg_ptr, format);
	n=vsnprintf(str,size,format,arg_ptr);
	va_end (arg_ptr);
	return n;
}
