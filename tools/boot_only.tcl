# boot_only.tcl - Re-start the app after power cycle / reset without re-flashing
# The image must already be in flash (0x10000000 = CM0+ vector table).
# Usage:
#   openocd -s <openocd scripts dir> -c "source [find interface/cmsis-dap.cfg]" ^
#           -c "source [find target/psoc6.cfg]" -c "init" ^
#           -c "source D:/cy8ckit-prj/tools/boot_only.tcl"

halt
mww 0x40210080 0x05FA0003

echo "=== booting CM0+ image from flash ==="
mem2array sp_arr 32 0x10000000 1
mem2array pc_arr 32 0x10000004 1
set sp [format "0x%08x" $sp_arr(0)]
set pc [format "0x%08x" $pc_arr(0)]
echo "SP=$sp PC=$pc"
reg sp $sp
reg pc $pc
resume
echo "=== app restarted ==="

shutdown