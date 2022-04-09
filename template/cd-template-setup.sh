#!/bin/bash

CDNAME=INSERT_CDNAME
ARG0=$0
FNAME=$1
BASENAME=$(basename $FNAME)
SERVICENAME="cdriver:$CDNAME"
DESTPATH=$APOLLO_DATA/$CDNAME/res
SEARCHSTR="#filetype,INSERT_CDFILETYPE"

if grep -q "$SEARCHSTR" "$FNAME"
then
   if [ -d $DESTPATH ];
   then
      cp -f $FNAME $DESTPATH
      chown apollo:apollo $DESTPATH/$BASENAME
      chmod 664 $DESTPATH/$BASENAME
      supervisorctl restart $SERVICENAME
      exit 0
   else
      echo Destination path $DESTPATH does not exist
      exit -1
   fi
else
   echo "$ARG0 is unable to process $FNAME"
   exit -1
fi
