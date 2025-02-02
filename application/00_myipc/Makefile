CROSS_COMPILE := arm-rockchip830-linux-uclibcgnueabihf-

CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)g++
CXX		= $(CROSS_COMPILE)g++
AR		= $(CROSS_COMPILE)ar
RANLIB	= $(CROSS_COMPILE)ranlib

LD		= $(CROSS_COMPILE)ld
AS		= $(CROSS_COMPILE)as
NM		= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump

DIRS := $(shell find ./ -maxdepth 5 -type d )

INCLUDE_DIRS := $(DIRS)
INCLUDE_DIRS += $(shell find ../rv1106_sdk/include -maxdepth 2 -type d )
INCLUDE_DIRS += $(shell find ../common -maxdepth 2 -type d )

VPATH := $(shell find ../common -maxdepth 2 -type d )

include config.mk
include ../common/Makefile

ifdef CONFIG_ENABLE_ROCKCHIP_YOLOV5
include ../common/yolov5/Makefile
endif

INCLUDE	:= $(patsubst %, -I %, $(INCLUDE_DIRS))

SOURCE := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))
SOURCE += $(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))

OBJS	:= $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

CFLAGS  := -Wall -Wfatal-errors -Os -g -Wl,--gc-sections -fno-common -lstdc++ $(INCLUDE)
CXXFLAGS:= $(CFLAGS)

LIBS += -L ../rv1106_sdk/lib
LIBS += -lrockit_full -lrockchip_mpp -ldrm -lrga -lrkaiq -lrtsp

ifdef CONFIG_ENABLE_ROCKCHIP_IVA
LIBS += -lrockiva
endif

ifdef CONFIG_ENABLE_ROCKCHIP_YOLOV5
LIBS += -lrknnmrt
endif

OBJS += $(obj-y)

# $(info $(DIRS))
# $(info $(SOURCE))
# $(info $(OBJS))

TARGET := $(notdir $(shell pwd))

.PHONY : everything objs clean distclean rebuild

everything : mk_config $(TARGET)
	$(STRIP) $(TARGET)
	# sshpass -p ' ' scp -P 30012 $(TARGET) root@jsfree.top:/userdata
	# sshpass -p ' ' scp -P 22 $(TARGET) root@192.168.1.21:/userdata

mk_config :
	@./convert_to_header.sh config.mk

objs :
	@echo $(obj-y)

rebuild: distclean everything

clean :
	@rm -rf *.*~
	@rm -rf *~
	@rm -rf *.so
	@rm -rf *.o
	@rm -rf $(OBJS)
	@rm -rf function_config.h

distclean : clean
	@rm -rf $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
