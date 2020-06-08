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
#ifndef __DUT_UTILITY_H_
#define __DUT_UTILITY_H_

int char2hex(const char *c, uint8_t *x);
int str2_char(const char *str, uint8_t *addr);
int int_num_check(char *data);
int16_t  asciitohex(char *data);
char *str_chr(char *d, char *s, int c);
int argc_len(char *s);
char *char_cut(char *d, char *s, int b , int e);

#endif
