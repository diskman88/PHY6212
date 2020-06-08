#!/bin/sh

PART_INFO_FILE=$1
LD_SCRIPT_IN=$2
LD_SCRIPT_OUT=$3

ADDRESS=`cat $PART_INFO_FILE | sed 's/[[:space:]]//g' | grep "name:prim" | awk -F'address:' '{printf "%s\n", $2}' | awk -F',' '{printf "%s\n", $1}'`
SIZE=`cat $PART_INFO_FILE | sed 's/[[:space:]]//g' | grep "name:prim" | awk -F'size:' '{printf "%s\n", $2}' | awk -F',' '{printf "%s\n", $1}'`
cat $LD_SCRIPT_IN | sed 's/PART_ADDR_REE/'"$ADDRESS"'/g' | sed 's/PART_SIZE_REE/'"$SIZE"'/g' > $LD_SCRIPT_OUT
