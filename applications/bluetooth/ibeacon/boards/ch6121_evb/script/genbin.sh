#flash
WORK_PATH=$1
rm -f $WORK_PATH/*.hex
arm-none-eabi-objcopy -j .text -j .rodata -j .ARM.exidx -O binary $WORK_PATH/../../yoc.elf $WORK_PATH/xprim
arm-none-eabi-objcopy -j .data_text -j .data -O binary $WORK_PATH/../../yoc.elf $WORK_PATH/prim
arm-none-eabi-objcopy -j .jmp_table -O binary $WORK_PATH/../../yoc.elf $WORK_PATH/jumptb
