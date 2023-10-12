#!/bin/bash
utg_pid=`pidof utg`
if [ $? -eq 0 ]; then
  kill ${utg_pid}
fi 
tusd_pids=`pidof tusd`
if [ $? -eq 0 ]; then
  kill ${tusd_pids}
fi 