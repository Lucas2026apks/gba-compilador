PATH := $(DEVKITARM)/bin:$(PATH)

PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

ARCH    := -mthumb -mthumb-interwork
CFLAGS  := $(ARCH) -O2 -Wall -fomit-frame-pointer
LDFLAGS := $(ARCH) -specs=gba.specs

TARGET  := granja
SRCS    := main.c
OBJS    := $(SRCS:.c=.o)

all: $(TARGET).gba

$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

%.gba: %.elf
	$(OBJCOPY) -v -O binary $< $@
	gbafix $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).gba

.PHONY: all clean
