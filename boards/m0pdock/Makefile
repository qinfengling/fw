TOP = $(shell pwd)/../..
BL_SDK_BASE = $(TOP)/../bouffalo_sdk

export BL_SDK_BASE
export TOP

CHIP = bl616
BOARD = bl616dk
CROSS_COMPILE = riscv64-unknown-elf-

# Note: COMX may change if there are other USB-serial devices or if enumeration
# happens while something is still using ACM0 or ACM1.
COMX = /dev/ttyACM1

SDK_LD = $(BL_SDK_BASE)/bsp/board/bl616dk/bl616_flash.ld

# add custom cmake definition
#cmake_definition+=-Dxxx=sss

include $(BL_SDK_BASE)/project.build

# Cmake always says "Built target sunela_bl616.elf", even if it did nothing.
# We therefore have to explicitly force linking when libsunela.a may have
# changed.

.PHONY: redo upload download download-all erase

redo:
	rm -f $(TOP)/sdk/build/build_out/sunela_bl616.elf
	$(MAKE)

# --- Flashing ----------------------------------------------------------------

FLASH = $(BL_FLASH_PROGRAM)  --interface uart --baudrate 2000000 --port=$(COMX) \
	--chipname bl616 --cpu_id m0 --flash
REGION_ALL = --start 0x0 --len 0x800000
REGION_DB = --start 0x400000 --len 0x200000

upload: $(TOP)/dummy.db
	$(FLASH) --write $(REGION_DB) --file $<
# --config=sdk/flash_prog_cfg.ini \

download:
	$(FLASH) --read $(REGION_DB) --file $(TOP)/flash.db

download-all:
	$(FLASH) --read $(REGION_ALL) --file $(TOP)/all.bin

erase:
	$(FLASH) --erase --whole_chip

# --- Debugging ---------------------------------------------------------------

# For convenience: invoke gdb on the target (SDK) executable
# E.g., for  info line *0x...

gdb:
	riscv64-unknown-elf-gdb $(TOP)/sdk/build/build_out/sunela_bl616.elf

# idem, for nm

nm:
	riscv64-unknown-elf-nm $(TOP)/sdk/build/build_out/sunela_bl616.elf

