#!/bin/sh

if [ ! -f "csi/Makefile" ]; then
	CSI_RTOS_PATH=`cat .git/config | grep yoc7 | awk -F'=' '{print $2}' | sed 's/IoT\/yoc7/csi/g'`
	BRANCH_NAME=`cat .git/HEAD | awk -F'/' '{print $3}'`

	echo "git clone $CSI_RTOS_PATH csi"
	git clone $CSI_RTOS_PATH csi
	cd csi
	git branch -a | grep $BRANCH_NAME$

	if [ $? = 0 ]; then
		git checkout $BRANCH_NAME
	else
		echo csi/$BRANCH_NAME not found, use master branch
	fi
else
	echo "csi exist"
fi
