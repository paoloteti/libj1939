#!/bin/sh

# configure PEAK PCAN-USB Pro

ip link set can0 down
ip link set can0 type can bitrate 500000
ip link set can0 up

ip link set can1 down
ip link set can1 type can bitrate 500000
ip link set can1 up
