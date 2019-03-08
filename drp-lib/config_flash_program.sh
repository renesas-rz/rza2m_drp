#!/bin/bash

OUT_BIN=drplib-config.bin
OUT_HEADER=drplib-config-api.h

# Address in QSPI Flash (last 4MB of 64MB QSPI Flash)
#DRP_FLASH_ADDR=0x23C00000

# Get address from drplib-config-api.h
DRP_FLASH_ADDR=$(cat $OUT_HEADER | grep "#define DRP_FLASH_ADDR" | sed "s/#define DRP_FLASH_ADDR //")

# Create a jlink script and execute it
echo "loadbin $OUT_BIN,$DRP_FLASH_ADDR" > /tmp/jlink_load.txt
echo "g" >> /tmp/jlink_load.txt
echo "exit" >> /tmp/jlink_load.txt


echo ""
echo "DRP_FLASH_ADDR=$DRP_FLASH_ADDR"
echo ""
echo "********************************************"
echo "* Reset your board into u-boot"
echo "********************************************"
echo ""
echo "Press Enter to continue..."
read DUMMY

JLinkExe -speed 15000 -if JTAG -jtagconf -1,-1 -device R7S921053VCBG -CommanderScript /tmp/jlink_load.txt
