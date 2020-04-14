#!/bin/sh

# create a virtual CAN port driver

modprobe vcan
ip link add dev vcan0 type vcan
ip link set up vcan0
