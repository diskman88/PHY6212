##
 # Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 #   http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
##

L_PATH := $(call cur-dir)

include $(DEFINE_LOCAL)

L_MODULE := liblibs

L_CFLAGS := -g -O0 

#misc
L_SRCS += misc/cJSON.c misc/cJPath.c \
            misc/ringbuffer.c \
            misc/udata.c \
            misc/crc16.c \
            misc/crc32.c \
            misc/path.c \
            misc/byte_rw.c

#L_SRCS += newlib_wrap/sys/write.c

L_SRCS += newlib_wrap/__dtostr.c
L_SRCS += newlib_wrap/fprintf.c
L_SRCS += newlib_wrap/getc.c
L_SRCS += newlib_wrap/getchar.c
L_SRCS += newlib_wrap/__isinf.c
L_SRCS += newlib_wrap/__isnan.c
L_SRCS += newlib_wrap/__lltostr.c
L_SRCS += newlib_wrap/__ltostr.c
L_SRCS += newlib_wrap/printf.c
L_SRCS += newlib_wrap/putc.c
L_SRCS += newlib_wrap/putchar.c
L_SRCS += newlib_wrap/puts.c
L_SRCS += newlib_wrap/snprintf.c
L_SRCS += newlib_wrap/sprintf.c
L_SRCS += newlib_wrap/vfprintf.c
L_SRCS += newlib_wrap/__v_printf.c
L_SRCS += newlib_wrap/vprintf.c
L_SRCS += newlib_wrap/vsnprintf.c
L_SRCS += newlib_wrap/vasprintf.c
L_SRCS += newlib_wrap/vsprintf.c
L_SRCS += newlib_wrap/string.c
L_SRCS += newlib_wrap/strdup.c
L_SRCS += newlib_wrap/string/strext.c
L_SRCS += newlib_wrap/string/strerror.c

include $(BUILD_MODULE)



