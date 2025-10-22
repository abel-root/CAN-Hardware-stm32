# STM32F0xx Makefile (Windows compatible)
#######################################

SOURCES = main.c system.c usbd_conf.c usbd_cdc_if.c usb_device.c usbd_desc.c interrupts.c system_stm32f0xx.c can.c slcan.c led.c error.c printf.c

# Get git version (no sed)
GIT_VERSION := $(shell git describe --abbrev=7 --dirty --always --tags)
GIT_REMOTE := $(shell git config --get remote.origin.url)

TARGET = canable-$(GIT_VERSION)
BUILD_DIR = build
LD_SCRIPT = STM32F042C6_FLASH.ld
USER_DEFS = -D HSI48_VALUE=48000000 -D HSE_VALUE=16000000
USER_INCLUDES =
USB_INCLUDES = -IMiddlewares/ST/STM32_USB_Device_Library/Core/Inc -IMiddlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
USER_CFLAGS = -Wall -g -ffunction-sections -fdata-sections -Os
ifneq ($(EXTERNAL_OSCILLATOR), 1)
USER_CFLAGS += -DINTERNAL_OSCILLATOR
endif
USER_LDFLAGS = -fno-exceptions -ffunction-sections -fdata-sections -Wl,--gc-sections
TARGET_DEVICE = STM32F042x6

#######################################
# binaries
#######################################
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar
RANLIB = arm-none-eabi-ranlib
SIZE = arm-none-eabi-size
OBJCOPY = arm-none-eabi-objcopy

# Custom mkdir command compatible with Windows
define MKDIR
@if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
endef

CORE = CORE_M0
CPU = cortex-m0
CUBELIB_BUILD_DIR = $(BUILD_DIR)/STM32Cube
CMSIS_PATH = Drivers/CMSIS
CMSIS_DEVICE_PATH = $(CMSIS_PATH)/Device/ST/STM32F0xx
DRIVER_PATH = Drivers/STM32F0xx_HAL_Driver
INCLUDES = -I$(CMSIS_PATH)/Include -I$(CMSIS_DEVICE_PATH)/Include -I$(DRIVER_PATH)/Inc -Iinc $(USB_INCLUDES) $(USER_INCLUDES)
DEFS = -D$(CORE) $(USER_DEFS) -D$(TARGET_DEVICE)
CFLAGS = $(DEFS) $(INCLUDES) -mcpu=$(CPU) -mthumb $(USER_CFLAGS) -DGIT_VERSION=\"$(GIT_VERSION)\" -DGIT_REMOTE=\"$(GIT_REMOTE)\"

all: $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

flash: all
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000:leave -D $(BUILD_DIR)/$(TARGET).bin

CUBELIB = $(CUBELIB_BUILD_DIR)/libstm32cube.a
CUBELIB_DRIVER_OBJS = $(addprefix $(CUBELIB_BUILD_DIR)/, $(patsubst %.c, %.o, $(notdir $(wildcard $(DRIVER_PATH)/Src/*.c))))

cubelib: $(CUBELIB)
$(CUBELIB): $(CUBELIB_DRIVER_OBJS)
	$(AR) rv $@ $(CUBELIB_DRIVER_OBJS)
	$(RANLIB) $@
$(CUBELIB_BUILD_DIR)/%.o: $(DRIVER_PATH)/Src/%.c | $(CUBELIB_BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $^
$(CUBELIB_BUILD_DIR):
	$(call MKDIR,$@)

#######################################
# USB library
#######################################
USB_MIDDLEWARE_PATH = ./Middlewares/ST/STM32_USB_Device_Library/
USB_BUILD_DIR = $(BUILD_DIR)/usb
USB_SOURCES += usbd_ctlreq.c usbd_ioreq.c usbd_core.c usbd_cdc.c
USB_OBJECTS += $(addprefix $(USB_BUILD_DIR)/,$(notdir $(USB_SOURCES:.c=.o)))

usb: $(USB_OBJECTS)

$(USB_BUILD_DIR)/%.o: $(USB_MIDDLEWARE_PATH)/Core/Src/%.c | $(USB_BUILD_DIR)
	$(CC) -Os $(CFLAGS) -c -o $@ $^
$(USB_BUILD_DIR)/%.o: $(USB_MIDDLEWARE_PATH)/Class/CDC/Src/%.c | $(USB_BUILD_DIR)
	$(CC) -Os $(CFLAGS) -c -o $@ $^
$(USB_BUILD_DIR):
	@echo Creating $(USB_BUILD_DIR)
	$(call MKDIR,$@)

#######################################
# Application
#######################################
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SOURCES:.c=.o))) $(BUILD_DIR)/startup_stm32f042x6.o
LIBS = -lstm32cube -lc -lm -lnosys
LDFLAGS = -T $(LD_SCRIPT) -L $(CUBELIB_BUILD_DIR) -static $(LIBS) $(USER_LDFLAGS)

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $^ $@
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $^ $@
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(USB_OBJECTS) $(CUBELIB)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(USB_OBJECTS) $(LDFLAGS) -Xlinker -Map=$(BUILD_DIR)/$(TARGET).map
	$(SIZE) $@
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Os -c -o $@ $^
$(BUILD_DIR)/%.o: src/%.s | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^
$(BUILD_DIR):
	$(call MKDIR,$@)

#######################################
# Clean
#######################################
clean:
		del /Q $(BUILD_DIR)\*.o 2>nul
		del /Q $(BUILD_DIR)\*.elf 2>nul
		del /Q $(BUILD_DIR)\*.hex 2>nul
		del /Q $(BUILD_DIR)\*.map 2>nul
		del /Q $(BUILD_DIR)\*.bin 2>nul

.PHONY: clean all cubelib usb
