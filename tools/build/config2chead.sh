#!/bin/sh

CONFIG_FILE=$1
C_HEAD_FILE=$2

if [ ! -f $CONFIG_FILE ]; then
if [ ! -f $C_HEAD_FILE/csi_config.h ]; then
    echo Create $C_HEAD_FILE
	echo '#ifndef __CSI_CONFIG_H__' >  $C_HEAD_FILE/csi_config.h
	echo '#define __CSI_CONFIG_H__' >> $C_HEAD_FILE/csi_config.h
	echo '#include "yoc_config.h"'  >> $C_HEAD_FILE/csi_config.h
	echo '#endif'                   >> $C_HEAD_FILE/csi_config.h
fi
    exit
fi

if [ ! -f "$C_HEAD_FILE" ]; then
    echo "[INFO] Configuration created"
    sh tools/build/genconfig.sh $CONFIG_FILE > $C_HEAD_FILE
else
    sh tools/build/genconfig.sh $CONFIG_FILE > $C_HEAD_FILE.tmp
    grep -Fvf $C_HEAD_FILE.tmp $C_HEAD_FILE
    ret1=$?
    grep -Fvf $C_HEAD_FILE $C_HEAD_FILE.tmp
    ret2=$?

    if [ "$ret1$ret2" = "11" ]; then
        echo "[INFO] Configuration unchanged"
        rm $C_HEAD_FILE.tmp
    else
        echo "[INFO] Configuration changed"
        mv $C_HEAD_FILE.tmp $C_HEAD_FILE
    fi
fi
