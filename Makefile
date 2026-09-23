CC      = arm-none-eabi-gcc
SIZE    = arm-none-eabi-size

CFLAGS  = -mcpu=cortex-m3 -mthumb -O2 -g -Wall -Wextra -Iinclude -ffreestanding -fno-common -fno-builtin
LDFLAGS = -mcpu=cortex-m3 -mthumb -T linker.ld -nostdlib -Wl,--gc-sections -Wl,-Map=phoenix.map

SRCS = src/phoenix.c src/sync.c src/startup.c src/main.c
ASMS = src/context_switch.S
OBJS = $(SRCS:.c=.o) $(ASMS:.S=.o)
TARGET = phoenix.elf

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) phoenix.map

run: $(TARGET)
	qemu-system-arm -M mps2-an385 -cpu cortex-m3 -nographic -kernel $(TARGET)

.PHONY: all clean run
