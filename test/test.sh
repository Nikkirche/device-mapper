#!/bin/bash
set -e
size=4096
make insmod
sudo dmsetup create zero1 --table "0 $size zero"
ls -al /dev/mapper/*
sudo dmsetup create dmp1 --table "0  $size dmp /dev/mapper/zero1"
ls -al /dev/mapper/*
echo "Started testing \n"
cat /sys/module/dmp/stat/volumes
echo $'Write test\n'
sudo dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1
echo $'\n'
cat /sys/module/dmp/stat/volumes
echo $'Read test\n'
sudo dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1
echo $'\n'
cat /sys/module/dmp/stat/volumes
