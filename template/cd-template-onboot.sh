#!/bin/bash

CDNAME=INSERT_CDNAME
getcap $APOLLO_DATA/$CDNAME/$CDNAME | grep -q $CDNAME
if [ $? -ne 0 ]; then 
   setcap cap_net_admin,cap_net_bind_service,cap_net_raw,cap_sys_resource,cap_sys_time=eip $APOLLO_DATA/$CDNAME/$CDNAME
   if [ $? -ne 0 ]; then 
      echo $0: Failed to set $APOLLO_DATA/$CDNAME/$CDNAME capabilities
   else
      echo $0: $APOLLO_DATA/$CDNAME/$CDNAME capabilities are successfully set
   fi
fi
