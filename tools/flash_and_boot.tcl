# flash_and_boot.tcl - Program a hex and manually boot the CM0+ image
# Usage:
#   openocd -c "set HEX D:/path/to/app.hex" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
# It programs the image, then reads the CM0+ vector table from flash
# (SP at 0x10000000, Reset at 0x10000004) and jumps to it.

halt
mww 0x40210080 0x05FA0003

echo "=== programming $HEX ==="
psoc6 sflash_restrictions 1
program $HEX verify
halt

echo "=== manual boot of CM0+ image ==="
set sp [mrw 0x10000000]
set pc [mrw 0x10000004]
echo "SP=0x$sp PC=0x$pc"
reg sp $sp
reg pc $pc
resume
echo "=== app started (LED should blink) ==="
sleep 500
mdw 0x40320080 1

shutdown
