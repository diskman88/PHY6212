/*
 * getchar.c - the file contains the functions getchar.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include "minilibc_stdio.h"
//#include <ansidef.h>
#include <stdarg.h>
#include <stdlib.h>

int getchar(void)
{
	return getc(stdin);
}

