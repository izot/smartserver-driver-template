##################################################################
# Please change the following lines to customize your IDL driver #
##################################################################

# CDNAME:         driver/protocol name
CDNAME=example
# CDDESC:         driver's description
CDDESC="$(CDNAME) driver for SmartServer IoT"
# CDDEVLIMIT:     driver's supported maximum device limit (10 for this example)
CDDEVLIMIT=10
# CDPROGRAMIDS:   driver's supported list of program_IDs in double quotes
#                 NOTES: starts with opening brace and close with closing brace
#                        preceed all double quotes(") with backlashes(\)
#                        no whitespace(s) between comma(s)
CDPROGRAMIDS={\"9B0001050004ED00\",\"9B0001050004EE00\"}
# CDVERSION:      driver version string
CDVERSION=1.00.004
# CDEXTENSION:    driver XIF file extension
CDEXTENSION=".xpl"
# CDCOPYRIGHT:    driver copy right string
CDCOPYRIGHT="Copyright 2019 - 2022 Dialog Semiconductor.  All Rights Reserved."
# CDMANUFACTURER: driver manufacturer
CDMANUFACTURER="Dialog Semiconductor, A Renesas Company"
# CDLICENSE:      driver license string/info
CDLICENSE="Dialog $(CDNAME) Custom Driver License"
# CDSOURCES:      driver's list of source files to compile & build
CDSOURCES=src/example.cpp
# CDCFLAGS:       list of C/C++ compilation flags such as -0g -ggdb (for debug build)
CDCFLAGS=-Og -ggdb
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
CFLAGS += -g $(CDCFLAGS) -Wall $(INCLUDES) -DCDNAME=\"$(CDNAME)\" -DCDDEVLIMIT=$(CDDEVLIMIT) -DCDPROGRAMIDS="$(CDPROGRAMIDS)"
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

