##
 # Copyright (C) 2017 C-SKY Microsystems Co., Ltd. All rights reserved.
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

#LIB_CSRC += $(MODULESDIR)/newlib_wrap/sys/write.c

LIB_CSRC += $(MODULESDIR)/newlib_wrap/__dtostr.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/fprintf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/getc.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/getchar.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/__isinf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/__isnan.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/__lltostr.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/__ltostr.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/printf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/putc.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/putchar.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/puts.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/snprintf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/sprintf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/vfprintf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/__v_printf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/vprintf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/vsnprintf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/vsprintf.c
LIB_CSRC += $(MODULESDIR)/newlib_wrap/vasprintf.c