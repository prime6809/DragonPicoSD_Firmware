#!/bin/bash

ROMDIR="./DragonFirmware"
ROMNAME="$ROMDIR/binaries/PicoMMC.ROM"

OUTPUT=picommcrom.c

echo "Building 6809 firmware"
echo 

pushd $ROMDIR
make 
popd

if [ -f ]
then
  echo "making $OUTPUT"
  echo
  xxd -n mmcrom -i ./DragonFirmware/binaries/PicoMMC.ROM | sed "s/unsigned char/const uint8_t __in_flash()/g" > $OUTPUT
  ls -la $OUTPUT
else
  echo "6809 build failed, please check"
fi
