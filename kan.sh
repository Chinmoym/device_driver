#!/bin/sh

sudo insmod /home/pi/chinmoy/gpio_ir.ko
echo 1 >> /sys/devices/platform/ir/button_power
