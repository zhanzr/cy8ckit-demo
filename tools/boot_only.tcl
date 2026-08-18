# boot_only.tcl - Re-start the app after power cycle / reset without re-flashing
# The image must already be in flash (0x10000000 = CM0+ vector table).
# Usage:
#   openocd -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" ^
#           -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" ^
#           -c "source D:/cy8ckit-prj/tools/boot_only.tcl"

halt
mww 0x40210080 0x05FA0003

echo "=== booting CM0+ image from flash ==="
set sp [mrw 0x10000000]
set pc [mrw 0x10000004]
echo "SP=0x$sp PC=0x$pc"
reg sp $sp
reg pc $pc
resume
echo "=== app restarted (LED should blink) ==="

shutdown
