# flash_stub_boot.tcl - erase the CM0+ region, program the XIP stub, boot it.
# The XIP stub inits SMIF and jumps the CM4 to 0x18000000.
# Usage:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#       -c "set HEX D:/.../cm0p_xip_stub.hex" \
#       -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" \
#       -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" \
#       -c "source D:/cy8ckit-prj/tools/flash_stub_boot.tcl"

halt
mww 0x40210080 0x05FA0003
echo "=== erase CM0+ region 0x10000000..0x10020000 ==="
flash erase_address 0x10000000 0x20000
echo "=== program stub $HEX ==="
psoc6 sflash_restrictions 1
program $HEX verify
halt

echo "=== boot the stub ==="
targets psoc6.cpu.cm0
halt
set sp0 [mrw 0x10000000]
set pc0 [mrw 0x10000004]
echo "CM0+ SP=0x$sp0 PC=0x$pc0"
reg sp $sp0
reg pc $pc0
resume
targets psoc6.cpu.cm4
halt
resume
sleep 300
shutdown
