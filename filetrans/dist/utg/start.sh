#!/bin/bash
PREFIX=`cat /proc/self/cgroup | head -1 | awk -F '/' '{print substr($3,1,13)}'`
ROOT_DIR=$(cd `dirname $0`; pwd)
export DATA_DIR=/data/utgfs
export MOUNT_DIR=/tmp
export PATH=$PATH:$ROOT_DIR
utg -data-dir $DATA_DIR -mount-dir $MOUNT_DIR >> /tmp/utg$PREFIX.log 2>&1 &