/*
 * Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 * @file     dut_utility.c
 * @brief    DUT Test Source File
 * @version  V1.0
 * @date     10. Dec 2019
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 */
#include "log.h"
#include <math.h>
#include <aos/aos.h>

int char2hex(const char *c, uint8_t *x)
{
    if (*c >= '0' && *c <= '9') {
        *x = *c - '0';
    } else if (*c >= 'a' && *c <= 'f') {
        *x = *c - 'a' + 10;
    } else if (*c >= 'A' && *c <= 'F') {
        *x = *c - 'A' + 10;
    } else {
        return -EINVAL;
    }

    return 0;
}

int str2_char(const char *str, uint8_t *addr)
{
    int i, j;
    uint8_t tmp;

    if (strlen(str) != 17) {
        return -EINVAL;
    }

    for (i = 0, j = 1; *str != '\0'; str++, j++) {
        if (!(j % 3) && (*str != ':')) {
            return -EINVAL;
        } else if (*str == ':') {
            i++;
            continue;
        }

        addr[i] = addr[i] << 4;

        if (char2hex(str, &tmp) < 0) {
            return -EINVAL;
        }

        addr[i] |= tmp;
    }

    return 0;
}

int int_num_check(char *data)
{
    if ((*data != '-')&&(*data != '+')&&((*data < 0x30)||(*data >0x39))){
        return  -1;
    }
    for (int i =1; *(data+i) != '\0'; i++) {
        if ((*(data+i) < 0x30)||(*(data+i) > 0x39)) {
            return  -1;
        }
    }
    return  0;
}

int16_t  asciitohex(char *data)
{
    int i;
    int ret_data=0;

    if(*data == '-'){
        for(i =1;i<strlen(data);i++){
            ret_data += ((*(data +i)-0x30)*pow(10,(strlen(data)-1-i)));
        }
        return -ret_data;
    }else if(*data == '+'){
        for(i =1;i<strlen(data);i++){
            ret_data += ((*(data +i)-0x30)*pow(10,(strlen(data)-1-i)));
        }
        return ret_data;
    }else{
        for(i =0;i<strlen(data);i++){
            ret_data += ((*(data +i)-0x30)*pow(10,(strlen(data)-1-i)));
        }
        return ret_data;
    }
}

char *str_chr(char *d, char *s, int c)
{
    int i = 0;
    char  *q = d;
    char  *p = s;

    for (i = 0; * (s + i) != (char) c; i++) {
        *(q++) = *(p++);

        if (*(s + i) == '\0') {
            return NULL;
        }
    }

    *(q++) = '\0';
    return  d;
}

int  argc_len(char *s)
{
    int i = 0;
    int j = 0;

    for (i = 0; * (s + i) != '\0'; i++) {
        if ((*(s + i) == ',') || (*(s + i) == '?') || (*(s + i) == '=')) {
            j++;
        }
    }
    return j;
}

char *char_cut(char *d, char *s, int b , int e)
{
    char *stc;

    if (b == '\0') {
        return NULL;
    }

    stc = strchr(s, b);

    if (stc == NULL) {
        printf("not execute\r\n");
        return NULL;
    }

    stc++;
    str_chr(d, stc, e);

    return d;//(char *)d;
}
