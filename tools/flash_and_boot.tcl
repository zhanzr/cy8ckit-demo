# flash_and_boot.tcl - Program a hex and manually boot both cores
# (for the ModusToolbox / Cypress openocd). Programs the image, then boots
# the CM0+ image from its vector table and resumes both cores.
# Usage:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#       -c "set HEX D:/.../<app>.hex" \
#       -c "adapter speed 1000" \
#       -c "source [find interface/kitprog3.cfg]" \
#       -c "source [find target/infineon/cy8c6xx.cfg]" \
#       -c "init" \
#       -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"

halt
mww 0x40210080 0x05FA0003
echo "=== programming $HEX ==="
psoc6 sflash_restrictions 1
program $HEX verify
halt

echo "=== manual boot: CM0+ vector from 0x10000000 ==="
targets psoc6.cpu.cm0
halt
set sp0 [mrw 0x10000000]
set pc0 [mrw 0x10000004]
echo "CM0+ SP=0x$sp0 PC=0x$pc0"
reg sp $sp0
reg pc $pc0
resume

echo "=== release CM4 too ==="
targets psoc6.cpu.cm4
halt
resume

sleep 300
shutdown
