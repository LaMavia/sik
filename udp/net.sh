#!/usr/bin/env bash

sudo ip address add "10.10.0.$1" dev eth1
sudo ifconfig eth1 netmask 255.255.255.0
sudo ifconfig eth1 broadcast 10.10.0.255
ip a
