#!/bin/bash

CDNAME=INSERT_CDNAME
CDPATH=$APOLLO_DATA/$CDNAME
SETUPFILE=$APOLLO_CONFD/scripts/setup.d/$CDNAME-xif-setup.sh
ONBOOTFILE=$APOLLO_CONFD/scripts/onboot.d/$CDNAME-onboot.sh
SUPERVISORFILE=/etc/supervisor/conf.d/$CDNAME.conf

MODE=$1

CURRENTSCRIPT="$0"

# Function that is called when the script exits:
function finish {
    echo "Securely shredding ${CURRENTSCRIPT}"; shred -u ${CURRENTSCRIPT};
}

if [ "$MODE" == "factory" ]; then
    if [ -f $SUPERVISORFILE ]; then
        echo $0: Deleting $SUPERVISORFILE and stopping the service
        rm -f $SUPERVISORFILE
    fi
    CDRIVERCONF=/etc/supervisor/conf.d/cdriver.conf
    if [ -f $CDRIVERCONF ]; then
        grep -qw $CDNAME $CDRIVERCONF
        if [ $? -eq 0 ]; then
            echo $0: updating $CDRIVERCONF to remove $CDNAME
            sed -i 's/\b,'"$CDNAME"'\b//' $CDRIVERCONF
            sed -i 's/\b'"$CDNAME"',\b//' $CDRIVERCONF
            sed -i '/\b'"$CDNAME"'\b/d' $CDRIVERCONF
            grep -q "programs:" $CDRIVERCONF
            if [ $? -ne 0 ]; then
                echo $0: removing $CDRIVERCONF
                rm $CDRIVERCONF
            fi
        else
            echo $0: $CDNAME is not found in programs under cdriver group in $CDRIVERCONF 
        fi
    else
        echo $0: $CDRIVERCONF not found.
    fi
    supervisorctl update
    if [ -d $CDPATH ]; then
        echo $0: Deleting $CDPATH
        rm -rf $CDPATH
    fi
    if [ -f $SETUPFILE ]; then
        echo $0: Deleting $SETUPFILE
        rm -f $SETUPFILE
    fi
    if [ -f $ONBOOTFILE ]; then
        echo $0: Deleting $ONBOOTFILE
        rm -f $ONBOOTFILE
    fi

    # exit with a call to the function, "finish":
    trap finish EXIT
fi
