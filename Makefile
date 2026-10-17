# --- CONFIGURACIÓN DEL PROYECTO GBA ---
TARGET		:= juego_gba
SOURCES		:= .
INCLUDES	:= .

# --- CONFIGURACIÓN DE COMPILACIÓN ---
ARCH		:= -mthumb -mthumb-interwork
CFLAGS		:= -Wall -Wextra -O2 $(ARCH) -I$(INCLUDES)
LDFLAGS		:= $(ARCH) -specs=gba.specs -ltonc

# Detección automática de devkitPro en Linux (GitHub Actions)
export DEVKITPRO ?= /opt/devkitpro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM
PREFIX		:= arm-none-eabi-

CC			:= $(PREFIX)gcc
LD			:= $(PREFIX)gcc
OBJCOPY		:= $(PREFIX)objcopy
GBAFIX		:= gbafix

CFILES		:= $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
OBJS		:= $(CFILES:.c=.o)

all: $(TARGET).gba

$(TARGET).gba: $(TARGET).elf
	@$(OBJCOPY) -O binary $< $@
	@$(GBAFIX) $@

$(TARGET).elf: $(OBJS)
	@$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(TARGET).gba $(TARGET).elf $(OBJS)
