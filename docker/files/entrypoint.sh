#!/bin/sh
PREFIX=""

PREFIX=`cat /proc/self/cgroup | head -1 | awk -F '/' '{print substr($3,1,13)}'`
rm -f /tmp/utgfs$PREFIX.pid
rm -f /tmp/hirefs$PREFIX.pid
/usr/bin/utgfsd -d -l 5 -p $PREFIX

sleep 5

/opt/utgfs/utg/start.sh

echo "nameserver 223.5.5.5" > /etc/resolv.conf
echo "nameserver 180.76.76.76" >> /etc/resolv.conf
echo "nameserver 119.29.29.29" >> /etc/resolv.conf
echo "nameserver 199.85.126.10" >> /etc/resolv.conf
echo "nameserver 8.8.8.8" >> /etc/resolv.conf

while :
do
  sleep 1
done
