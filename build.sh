#!/bin/sh

chmod +x tools/build/product

DEFCONFIG=$1
APPLICATION=$2
J=$3

if [ ! -f $DEFCONFIG ]; then
	echo $DEFCONFIG" not exist!"
fi

if [ ! -d $APPLICATION ]; then
	echo $APPLICATION" not exist!"
fi

if [ -f defconfig ]; then
	diff defconfig $DEFCONFIG > /dev/null
	if [ $? != 0 ]; then
		cp $DEFCONFIG defconfig
		make clean
	fi
else
	cp $DEFCONFIG defconfig
	make clean
fi

make auto_config
make csi_rtos -$J
make -$J
make

cp $APPLICATION/defconfig_ch6121_evb $APPLICATION/defconfig
make -C $APPLICATION
