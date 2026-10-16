# Nombre de tu archivo de salida (.gba)
TARGET		:= juego_rpg_terraria
BUILD		:= build
SOURCES		:= src
INCLUDES	:= include

# Configuración de devkitARM
export PATH	:= $(DEVKITARM)/bin:$(PATH)
PREFIX		:= arm-none-eabi-
CC			:= $(PREFIX)gcc
LD			:= $(PREFIX)gcc

ARCH		:= -mthumb -mthumb-interwork
CFLAGS		:= -g -Wall -O2 -ffunction-sections -fdata-sections $(ARCH) -specs=gba.specs
LDFLAGS		:= -specs=gba.specs $(ARCH) -Wl,--gc-sections

# Archivos fuente
CFILES		:= $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
OBJECTS		:= $(addprefix $(BUILD)/,$(notdir $(CFILES:.c=.o)))

export VPATH	:= $(foreach dir,$(SOURCES),$(dir))

all: $(BUILD) $(TARGET).gba

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).gba: $(TARGET).elf
	arm-none-eabi-objcopy -O binary $< $@

$(TARGET).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).gba
