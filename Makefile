# --- CONFIGURACIÓN DEL PROYECTO ---
TARGET		:= gba_mining_game
SOURCES		:= .
INCLUDES	:= .
DATA		:= .

# --- ARCHIVOS Y HERRAMIENTAS ---
ARCH		:= -mthumb -mthumb-interwork
CFLAGS		:= -Wall -Wextra -O2 -g $(ARCH) $(INCLUDE)
CXXFLAGS	:= $(CFLAGS)
LDFLAGS		:= -g $(ARCH)

# Cargar las herramientas estándar de devkitARM (si usas devkitPro)
ifeq ($(strip $(DEVKITPRO)),)
$(error "Por favor, asegúrate de tener configurada la variable DEVKITPRO.")
endif

export PATH	:= $(DEVKITPRO)/devkitARM/bin:$(PATH)
PREFIX		:= arm-none-eabi-
CC			:= $(PREFIX)gcc
LD			:= $(PREFIX)gcc
OBJCOPY		:= $(PREFIX)objcopy
GBAFIX		:= gbafix

# --- REGLAS DE COMPILACIÓN ---
CFILES		:= $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
OBJS		:= $(CFILES:.c=.o)

all: $(TARGET).gba

$(TARGET).gba: $(TARGET).elf
	@$(OBJCOPY) -O binary $< $@
	@$(GBAFIX) $@

$(TARGET).elf: $(OBJS)
	@$(LD) $(LDFLAGS) $(OBJS) -ltonc -specs=gba.specs -o $@

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(TARGET).gba $(TARGET).elf $(OBJS)
