/*
 * putchar.c - the file contains the functions putchar.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include "minilibc_stdio.h"

int putchar(int c)
{
	putc(c,stdout);

    return 0;
}

