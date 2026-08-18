CC = D:/arm-none-eabi-tc/bin/arm-none-eabi-gcc
OBJCOPY = D:/arm-none-eabi-tc/bin/arm-none-eabi-objcopy

PDL = D:/board_database/main-cy8ckit-062/mtb-pdl-cat1-release-v3.23.0
CMSIS = D:/modubus_wks/mtb_shared/cmsis/release-v5.8.0/Core/Include
UTILS = D:/modubus_wks/mtb_shared/core-lib/release-v1.8.0/include
DEVICES = $(PDL)/devices/COMPONENT_CAT1A

DEFINES = -DCY8C6347BZI_BLD53 \
          -DCOMPONENT_PSOC6_01 \

          -DCY_IPC_DEFAULT_CFG_DISABLE \
          -DCY_CORTEX_M4_APPL_ADDR=0x10020000

INC = -Isystem \
      -Iext/cmsis \
      -I$(CMSIS) \
      -I$(UTILS) \
      -I$(DEVICES)/include \
      -I$(DEVICES)/include/ip \
      -I$(PDL)/drivers/include

COMMON_CFLAGS = -mthumb -Os -g -Wall -ffunction-sections -fdata-sections \
                $(DEFINES) $(INC)

CM0P_CFLAGS = $(COMMON_CFLAGS) -mcpu=cortex-m0plus -mfloat-abi=soft
CM4_CFLAGS  = $(COMMON_CFLAGS) -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

BUILD = build

all: $(BUILD)/combined.hex
	@echo "=== Build complete ==="
	@$(SIZE) $(BUILD)/cm0p.elf
	@$(SIZE) $(BUILD)/cm4.elf

# --- CM0+ ---
CM0P_OBJS  = $(BUILD)/cm0p_main.o \
             $(BUILD)/cm0p_uart.o \
             $(BUILD)/cm0p_sysinit.o \
             $(BUILD)/cm0p_gpio.o \
             $(BUILD)/cm0p_scb_uart.o \
             $(BUILD)/cm0p_scb_common.o \
             $(BUILD)/cm0p_sysclk.o \
             $(BUILD)/cm0p_systick.o \
             $(BUILD)/cm0p_wdt.o \
             $(BUILD)/cm0p_sysint.o \
             $(BUILD)/cm0p_syslib.o \
             $(BUILD)/cm0p_ipc_drv.o \
             $(BUILD)/cm0p_ipc_sema.o \
             $(BUILD)/cm0p_ipc_pipe.o \
             $(BUILD)/cm0p_flash.o

$(BUILD)/cm0p_main.o: cm0p/main.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -c $< -o $@

$(BUILD)/cm0p_uart.o: cm0p/uart.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -c $< -o $@

$(BUILD)/cm0p_sysinit.o: system/system_psoc6_cm0plus.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -c $< -o $@

$(BUILD)/cm0p_gpio.o: $(PDL)/drivers/source/cy_gpio.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_scb_uart.o: $(PDL)/drivers/source/cy_scb_uart.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_scb_common.o: $(PDL)/drivers/source/cy_scb_common.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_sysclk.o: $(PDL)/drivers/source/cy_sysclk.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_systick.o: $(PDL)/drivers/source/cy_systick.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_wdt.o: $(PDL)/drivers/source/cy_wdt.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_sysint.o: $(PDL)/drivers/source/cy_sysint.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_syslib.o: $(PDL)/drivers/source/cy_syslib.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_ipc_drv.o: $(PDL)/drivers/source/cy_ipc_drv.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_ipc_sema.o: $(PDL)/drivers/source/cy_ipc_sema.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_ipc_pipe.o: $(PDL)/drivers/source/cy_ipc_pipe.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p_flash.o: $(PDL)/drivers/source/cy_flash.c | $(BUILD)
	$(CC) $(CM0P_CFLAGS) -DCY_CPU_CORTEX_M0P -c $< -o $@

$(BUILD)/cm0p.elf: $(CM0P_OBJS)
	$(CC) -mcpu=cortex-m0plus -mthumb -T linker/cm0plus.ld -Wl,--gc-sections -Wl,-Map=$(BUILD)/cm0p.map -o $@ $^ -lm -lc -lnosys

# --- CM4 ---
CM4_OBJS = $(BUILD)/cm4_main.o \
           $(BUILD)/cm4_sysinit.o \
           $(BUILD)/cm4_gpio.o \
           $(BUILD)/cm4_systick.o \
           $(BUILD)/cm4_wdt.o \
           $(BUILD)/cm4_sysint.o \
           $(BUILD)/cm4_syslib.o

$(BUILD)/cm4_main.o: cm4/main.c | $(BUILD)
	$(CC) $(CM4_CFLAGS) -c $< -o $@

$(BUILD)/cm4_sysinit.o: system/system_psoc6_cm4.c | $(BUILD)
	$(CC) $(CM4_CFLAGS) -c $< -o $@

$(BUILD)/cm4_gpio.o: $(PDL)/drivers/source/cy_gpio.c | $(BUILD)
	$(CC) $(CM4_CFLAGS) -c $< -o $@

$(BUILD)/cm4_systick.o: $(PDL)/drivers/source/cy_systick.c | $(BUILD)
	$(CC) $(CM4_CFLAGS) -c $< -o $@

$(BUILD)/cm4_wdt.o: $(PDL)/drivers/source/cy_wdt.c | $(BUILD)
	$(CC) $(CM4_CFLAGS) -c $< -o $@

$(BUILD)/cm4_sysint.o: $(PDL)/drivers/source/cy_sysint.c | $(BUILD)
	$(CC) $(CM4_CFLAGS) -c $< -o $@

$(BUILD)/cm4_syslib.o: $(PDL)/drivers/source/cy_syslib.c | $(BUILD)
	$(CC) $(CM4_CFLAGS) -c $< -o $@

$(BUILD)/cm0p.bin: $(BUILD)/cm0p.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/m0p_image.o: $(BUILD)/cm0p.bin | $(BUILD)
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm \
		--rename-section .data=.cy_m0p_image \
		--redefine-sym _binary_cm0p_bin_start=_binary_cm0p_image_start \
		--redefine-sym _binary_cm0p_bin_end=_binary_cm0p_image_end \
		--redefine-sym _binary_cm0p_bin_size=_binary_cm0p_image_size \
		$< $@

$(BUILD)/cm4.elf: $(CM4_OBJS) $(BUILD)/m0p_image.o
	$(CC) -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
		-T linker/cm4.ld -Wl,--gc-sections -Wl,-Map=$(BUILD)/cm4.map -o $@ $^ -lm -lc -lnosys

# --- Combined hex ---
$(BUILD)/combined.hex: $(BUILD)/cm4.elf
	$(OBJCOPY) -O ihex $< $@

# --- Flash ---
flash: $(BUILD)/combined.hex
	openocd -f interface/stlink.cfg -f target/psoc6.cfg \
		-c "init; reset halt; flash write_image erase $<; reset run; exit"

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all flash clean
