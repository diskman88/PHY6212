/*
 * puts.c - the file contains the functions puts.
 *
 * Copyright (C): 2012 Hangzhou C-SKY Microsystem Co.,LTD.
 * Author: yu chaojun  (chaojun_yu@c-sky.com)
 * Date: 2012-5-9
 */
#include "minilibc_stdio.h"

int puts(const char *s)
{
   while(*s !='\0')
   {
       fputc(*s, (void *)-1);
       s++;
   }
   fputc('\n', (void *)-1);
   return 0;
}
