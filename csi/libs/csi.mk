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
INCDIR += -I$(LIBSDIR)/stdio
INCDIR += -I$(LIBSDIR)/string
INCDIR += -I$(MODULESDIR)/comm/include
INCDIR += -I$(LIBSDIR)/trace/include

ifndef CONFIG_NUTTXMM_NONE

LIB_CSRC += \
          $(LIBSDIR)/libc/malloc.c \
          $(LIBSDIR)/libc/_init.c \
          $(LIBSDIR)/mm/mm_malloc.c \
          $(LIBSDIR)/mm/mm_free.c \
          $(LIBSDIR)/mm/mm_leak.c \
          $(LIBSDIR)/mm/mm_size2ndx.c \
          $(LIBSDIR)/mm/mm_addfreechunk.c \
          $(LIBSDIR)/mm/mm_initialize.c \
          $(LIBSDIR)/mm/mm_mallinfo.c \
          $(LIBSDIR)/mm/lib_mallinfo.c \
          $(LIBSDIR)/mm/dq_addlast.c \
          $(LIBSDIR)/mm/dq_rem.c
endif

ifdef CONFIG_OS_TRACE
CSRC += \
    $(LIBSDIR)/trace/trcBase.c \
    $(LIBSDIR)/trace/trcHardwarePort.c \
    $(LIBSDIR)/trace/trcKernel.c \
    $(LIBSDIR)/trace/trcTrigger.c \
    $(LIBSDIR)/trace/trcKernelPort.c \
    $(LIBSDIR)/trace/trcUser.c
endif

ifeq ($(CONFIG_OS_COMM), y)
include $(LIBSDIR)/comm/csi.mk
endif
