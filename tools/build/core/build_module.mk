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

MK_CFLAGS += $(G_CFLAGS)
CPRE := @
ifeq ($(V),1)
CPRE :=
endif

L_INCS += \
    $(MK_OUT_PATH) \
    $(MK_OUT_PATH)/$(L_MODULE)

L_OBJS := $(L_SRCS:%c=$(MK_OUT_PATH)/$(L_MODULE)/%o)
L_DEPS := $(L_OBJS:%o=%d)
$(L_OBJS): PRIVATE_L_CFLAGS:= -c $(L_CFLAGS) $(L_INCS:%=-I%)
$(L_OBJS): $(MK_OUT_PATH)/$(L_MODULE)/%.o: $(L_PATH)/%.c
	@mkdir -p $(dir $@)
	@echo [CC] $<
	$(CPRE)$(CC) -MP -MMD  $(MK_CFLAGS) $(PRIVATE_L_CFLAGS) -o $@ $<
$(L_OBJS): $(L_JS_H) $(L_BYTE_H)

sinclude $(L_DEPS)

L_ASM_OBJS:= $(L_ASM_SRCS:%S=$(MK_OUT_PATH)/$(L_MODULE)/%o)
$(L_ASM_OBJS): PRIVATE_L_CFLAGS:= -c $(L_CFLAGS) $(L_INCS:%=-I%)
$(L_ASM_OBJS): $(MK_OUT_PATH)/$(L_MODULE)/%.o: $(L_PATH)/%.S
	@mkdir -p $(dir $@)
	@echo [AS] $<
	$(CPRE)$(CC) $(PRIVATE_L_CFLAGS) $(MK_CFLAGS) -o $@ $<

MK_L_OBJS += $(L_OBJS) $(L_ASM_OBJS)

L_TARGET:= $(MK_OUT_PATH_LIB)/$(L_MODULE).a
$(L_TARGET): PRIVATE_L_LDFLAGS:= -r
$(L_TARGET): $(L_OBJS) $(L_ASM_OBJS)
	@mkdir -p $(dir $@)
	@echo [AR] $@
	$(CPRE) $(AR) $(PRIVATE_L_LDFLAGS) -o $@ $^

 MK_L_LIBS += $(L_TARGET)

TMP_PRE_LIBS_SRC:=$(L_PRE_LIBS:%=$(L_PATH)/%)
TMP_NAME:= $(notdir $(L_PRE_LIBS))
L_PRE_LIBS:=$(TMP_NAME:%=$(MK_OUT_PATH_LIB)/prelibs/%)

MK_L_PRE_LIBS += $(L_PRE_LIBS)
MK_L_PRE_LIBS_SRC += $(TMP_PRE_LIBS_SRC)
