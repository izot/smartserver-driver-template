#!/bin/bash

IDI_PATH=$1
CDNAME=$2
CDDESC=$3
CDDEVLIMIT=$4
CDVERSION=$5
CDFILETYPE=$6
CDEXTENSION=$7
CDCOPYRIGHT=$8
CDMANUFACTURER=$9
CDLICENSE=${10}

RELEASE_PATH=$IDI_PATH/build/release
RELEASE=$RELEASE_PATH/$CDNAME
GLPO_NAME=${CDNAME}_driver.glpo
IMAGE_NAME=image.zip
cp -f $IDI_PATH/template/manifest $RELEASE_PATH/glpo/
cp -f $IDI_PATH/template/cd-template.conf $RELEASE_PATH/image/$CDNAME.conf
cp -f $IDI_PATH/template/cdriver.conf $RELEASE_PATH/image/cdriver.conf
cp -f $IDI_PATH/template/cd-template-idl.conf $RELEASE_PATH/image/$CDNAME-idl.conf
cp -f $IDI_PATH/template/cd-template-setup.sh $RELEASE_PATH/image/$CDNAME-xif-setup.sh
cp -f $IDI_PATH/template/cd-template-onboot.sh $RELEASE_PATH/image/$CDNAME-onboot.sh
cp -f $IDI_PATH/template/cd-template-reset.sh $RELEASE_PATH/image/$CDNAME-reset.sh
cp -f $IDI_PATH/template/setup $RELEASE_PATH/image/
cp -f $RELEASE $RELEASE_PATH/image/
chmod u=rwx,g=rwx,o=rx $RELEASE_PATH/image/setup
chmod u=rwx,g=rwx,o=rx $RELEASE_PATH/image/$CDNAME
chmod u=rwx,g=rwx,o=rx $RELEASE_PATH/image/$CDNAME-xif-setup.sh
chmod u=rwx,g=rwx,o=rx $RELEASE_PATH/image/$CDNAME-onboot.sh
chmod u=rw,g=rw,o=r $RELEASE_PATH/image/$CDNAME.conf
chmod u=rw,g=rw,o=r $RELEASE_PATH/image/cdriver.conf
sed -i ':a;s/INSERT_CDNAME/'"$CDNAME"'/;ta;' $RELEASE_PATH/image/$CDNAME.conf
sed -i 's/INSERT_CDNAME/'"$CDNAME"'/' $RELEASE_PATH/image/cdriver.conf
sed -i ':a;s/INSERT_CDNAME/'"$CDNAME"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDDESC/'"$CDDESC"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDDEVLIMIT/'$CDDEVLIMIT'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDVERSION/'"$CDVERSION"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDFILETYPE/'"$CDFILETYPE"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDEXTENSION/'"$CDEXTENSION"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDCOPYRIGHT/'"$CDCOPYRIGHT"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDMANUFACTURER/'"$CDMANUFACTURER"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf
sed -i ':a;s/INSERT_CDLICENSE/'"$CDLICENSE"'/;ta;' $RELEASE_PATH/image/$CDNAME-idl.conf

sed -i ':a;s/INSERT_CDNAME/'"$CDNAME"'/;ta;' $RELEASE_PATH/image/$CDNAME-xif-setup.sh
sed -i ':a;s/INSERT_CDFILETYPE/'"$CDFILETYPE"'/;ta;' $RELEASE_PATH/image/$CDNAME-xif-setup.sh
sed -i ':a;s/INSERT_CDNAME/'"$CDNAME"'/;ta;' $RELEASE_PATH/image/$CDNAME-onboot.sh
sed -i ':a;s/INSERT_CDNAME/'"$CDNAME"'/;ta;' $RELEASE_PATH/image/$CDNAME-reset.sh
sed -i ':a;s/INSERT_CDNAME/'"$CDNAME"'/;ta;' $RELEASE_PATH/image/setup
zip -jr $RELEASE_PATH/glpo/$IMAGE_NAME $RELEASE_PATH/image/*
IMAGE_SHA256=($(sha256sum $RELEASE_PATH/glpo/$IMAGE_NAME))
sed -i 's/INSERT_CDVERSION/'"$CDVERSION"'/' $RELEASE_PATH/glpo/manifest
sed -i 's/INSERT_CDMANUFACTURER/'"$CDMANUFACTURER"'/' $RELEASE_PATH/glpo/manifest
sed -i 's/INSERT_CDDESC/'"$CDDESC"'/' $RELEASE_PATH/glpo/manifest
sed -i 's/INSERT_SHA256/'"$IMAGE_SHA256"'/' $RELEASE_PATH/glpo/manifest
zip -jr $RELEASE_PATH/$GLPO_NAME $RELEASE_PATH/glpo/*
