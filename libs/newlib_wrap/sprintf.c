/*
 * sprintf.c - the file contains the functions sprintf.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include "minilibc_stdio.h"
#include<stdarg.h>
#include <stdlib.h>

//libc_hidden_proto(vsprintf)

int sprintf(char *dest,const char *format,...)
{
	int n;
	va_list arg_ptr;
	va_start(arg_ptr, format);
	n=vsprintf(dest,format,arg_ptr);
	va_end (arg_ptr);
	return n;
}
