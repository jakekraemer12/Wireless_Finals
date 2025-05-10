#!/bin/bash
#ESP_MAC="A0:DD:6C:74:7F:26"
#Characteristic_HANDLE=0x002a
#MOTION_HEX="4d 6f 74 69 6f 6e"   #HEX FOR ACII "MOTION"

#Location of python script on rapberry pi
PY="/usr/bin/python3"

#This bash script uses gatttool to connect to the ESP MAC's BLE connection and parses the motion characteristic looking for "motion" to 
#begin the process of taking a photo. When script is finished runnning 
while true
do
    if [ "$(sudo gatttool -b A0:DD:6C:74:7F:26 --char-read -a 0x002a | awk -F':' '{print $NF}')" = " 4d 6f 74 69 6f 6e " ]
    then
        echo "Motion detected... Taking photo"
        "$PY" /home/jakekraem/capture_app/capture_photo.py
        sleep 2
    else
        echo "$(sudo gatttool -b A0:DD:6C:74:7F:26 --char-read -a 0x002a | awk -F':' '{print $NF}')"
    fi 
done