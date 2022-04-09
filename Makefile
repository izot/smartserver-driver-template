#######################################################################
# Please change the following four lines to customize your IDL driver #
#######################################################################

CDNAME=test
CDDESC="$(CDNAME) driver protocol engine for SmartServer IoT"
CDLIMIT=127
CDVERSION=1.00.004
CDFILETYPE="$(CDNAME)_xif"
CDEXTENSION=".csv"
CDCOPYRIGHT="Copyright 2019 - 2022 Dialog Semiconductor.  All Rights Reserved."
CDMANUFACTURER="Dialog Semiconductor, A Renesas Company"
CDLICENSE="Dialog $(CDNAME) Custom Driver License"

CDSOURCES=src/example.cpp
CDLIBS=




##################################################
# Please do not change the rest of this Makefile #
##################################################

IDI_PATH=$(shell pwd)
BUILD_PATH=$(IDI_PATH)/build
RELEASE_PATH=$(BUILD_PATH)/release
RELEASE=$(RELEASE_PATH)/$(CDNAME)
GLPO_NAME=$(CDNAME)_driver.glpo
IMAGE_NAME=image.zip
CROSS_COMPILER=arm-linux-gnueabihf-
# CC=arm-linux-gnueabihf-gcc
# CXX=arm-linux-gnueabihf-g++
INCLUDES = -I$(IDI_PATH)/src
INCLUDES += -I$(IDI_PATH)/src/idl/include
INCLUDES += -L$(IDI_PATH)/src/idl/lib
CFLAGS += -g -Wall $(INCLUDES) -DCDNAME=\"$(CDNAME)\" -DCDRXTHREAD=\"$(CDRXTHREAD)\" -DCDRXFUNCTION=\"$(CDRXFUNCTION)\"
LIBS=-lidl -lmosquitto -lpthread -lrt $(CDLIBS)

CSRC = src/main.cpp $(CDSOURCES)

all: $(RELEASE) $(DEBUG)

$(RELEASE): $(CSRC)
	@mkdir -p $(RELEASE_PATH)
	$(CROSS_COMPILER)$(CXX) $(CFLAGS) -o $(RELEASE) $(CSRC) $(LIBS)
	@mkdir -p $(RELEASE_PATH)/glpo
	@mkdir -p $(RELEASE_PATH)/image
	./build.sh $(IDI_PATH) $(CDNAME) $(CDDESC) $(CDLIMIT) $(CDVERSION) $(CDFILETYPE) $(CDEXTENSION) $(CDCOPYRIGHT) $(CDMANUFACTURER) $(CDLICENSE)


clean:
	    rm -rf $(BUILD_PATH) $(RELEASE_PATH)/glpo $(RELEASE_PATH)/image $(DEBUG) 

.PHONY: clean

