#!/bin/bash

PWD=`pwd`

XMOS_DRV_DIR=$PWD"/drivers/mtd/devices/"
XMOS_SCRIPT_DIR=$PWD"/scripts/xmos/"

FIRMWARE_FBINARY=$XMOS_SCRIPT_DIR"xmosfactory"
FIRMWARE_UBINARY=$XMOS_SCRIPT_DIR"xmosupgrade"
FIRMWARE_BINARY_HEADER=$XMOS_DRV_DIR"xmosdata.h"

# array of model names
MODELS[0]="AK240_HW_ES"
MODELS[1]="AK240_HW_TP1"
MODELS[2]="AK240_HW_TP2"
MODELS[3]="AK1000"
MODELS[4]="AK500N_HW"
MODELS[5]="AK380_HW"
MODELS[6]="AK300_HW"
MODEL_CNT=7

IDX=0;

for ((IDX=0; IDX < MODEL_CNT; IDX++))
do
	grep "${MODELS[$IDX]}" ${PWD}/arch/arm/mach-tcc893x/include/mach/ak_target.h
	if [ "$?" = 0 ]
	then
		break
	fi
done

case "$IDX" in
	# AK240_HW_ES / AK240_HW_TP1
	"0"|"1")
		FIRMWARE_UBINARY=$XMOS_SCRIPT_DIR"xmosupgrade"
		;;

	# AK240_HW_TP2
	"2")
		FIRMWARE_UBINARY=$XMOS_SCRIPT_DIR"xmosupgrade_tp2"
		;;

	# AK1000
	"3")
		FIRMWARE_UBINARY=$XMOS_SCRIPT_DIR"xmosupgrade_tp2"
		;;

	# AK500N
	"4")
		FIRMWARE_UBINARY=$XMOS_SCRIPT_DIR"xmosupgrade_500n"
		;;

	# AK380
	"5")
		FIRMWARE_UBINARY=$XMOS_SCRIPT_DIR"xmosupgrade_tp2"
		;;

	# AK300
	"6")
		FIRMWARE_UBINARY=$XMOS_SCRIPT_DIR"xmosupgrade_300"
		;;
		
	# not support model name
	*)
		echo "This model does not support XMOS."
		exit 0
		;;
esac

echo "XMOS Upgrade firmware : "$FIRMWARE_UBINARY

if [ -e $FIRMWARE_UBINARY ]
then
	echo "unsigned char factory_data[] = {" > $FIRMWARE_BINARY_HEADER
	hexdump -v -e '16/1 "0x%02x, "' -e '"\n"' $FIRMWARE_FBINARY >> $FIRMWARE_BINARY_HEADER
	echo "};" >> $FIRMWARE_BINARY_HEADER
	
	echo "unsigned char upgrade_data[] = {" >> $FIRMWARE_BINARY_HEADER
	hexdump -v -e '16/1 "0x%02x, "' -e '"\n"' $FIRMWARE_UBINARY >> $FIRMWARE_BINARY_HEADER
	echo "};" >> $FIRMWARE_BINARY_HEADER
else
	echo $FIRMWARE_UBINARY " is not exist"
fi
