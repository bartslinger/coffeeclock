#!/bin/sh

# copy to banana pi
scp build-pro/coffeeclock.hex bart@192.168.114.141:coffeeclock/build-pro

# upload from pi to arduino
ssh bart@192.168.114.141 '
killall python
sleep 1s
cd coffeeclock
./upload.sh
sleep 1s
echo "starting"
nohup ./coffeeclock.py > /dev/null 2>&1 &
'
