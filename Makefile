##################################################################
# Please change the following lines to customize your IDL driver #
##################################################################

# CDNAME:         driver/protocol name, all in lower case (no CamelCase naming convention)
CDNAME=example
# CDDESC:         driver's description
CDDESC="$(CDNAME) driver for SmartServer IoT"
# CDDEVLIMIT:     driver's supported maximum device limit (10 for this example)
CDDEVLIMIT=10
# CDVERSION:      driver version string
CDVERSION=1.00.001
# CDEXTENSION:    driver XIF file extension
CDEXTENSION=".xpl"
# CDMANUFACTURER: driver manufacturer
CDMANUFACTURER="ACME Corporation"
# CDCOPYRIGHT:    driver copyright string
CDCOPYRIGHT="Copyright 2019 - 2022 $(CDMANUFACTURER).  All Rights Reserved."
# CDLICENSE:      driver license string/info
CDLICENSE="$(CDNAME) Custom Driver License"
# CDSOURCES:      driver's list of source files to compile & build
CDSOURCES=src/example.cpp src/eti.cpp
# CDINCETI:	      to exclude ETI example, clear the following line
CDINCETI=-DINCLUDE_ETI
# CDCFLAGS:       list of C/C++ compilation flags such as -0g -ggdb (for debug build)
CDCFLAGS=
# CDLIBS:         list of extra/thirdparty libraries used by the linker
CDLIBS=

##################################################
# Please do not change the rest of this Makefile #
##################################################

CDFILETYPE="$(CDNAME)_xif"
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
CFLAGS += $(CDCFLAGS) -Wall $(INCLUDES) -DCDNAME=\"$(CDNAME)\" -DCDDEVLIMIT=$(CDDEVLIMIT) $(CDINCETI)
LIBS=-lidl -lmosquitto -lpthread -lrt $(CDLIBS)

CSRC = src/main.cpp $(CDSOURCES)

all: $(RELEASE) $(DEBUG)

$(RELEASE): $(CSRC)
	@mkdir -p $(RELEASE_PATH)
	$(CROSS_COMPILER)$(CXX) $(CFLAGS) -o $(RELEASE) $(CSRC) $(LIBS)
	@mkdir -p $(RELEASE_PATH)/glpo
	@mkdir -p $(RELEASE_PATH)/image
	./build.sh $(IDI_PATH) $(CDNAME) $(CDDESC) $(CDDEVLIMIT) $(CDVERSION) $(CDFILETYPE) $(CDEXTENSION) $(CDCOPYRIGHT) $(CDMANUFACTURER) $(CDLICENSE)


clean:
	    rm -rf $(BUILD_PATH) $(RELEASE_PATH)/glpo $(RELEASE_PATH)/image $(DEBUG) 

.PHONY: clean

