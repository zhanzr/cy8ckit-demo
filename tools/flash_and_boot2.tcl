# flash_and_boot2.tcl - program a hex and boot via reset run (proper PSoC6 boot)
# Usage:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#       -c "set HEX D:/.../<app>.hex" \
#       -c "adapter speed 1000" \
#       -c "source [find interface/kitprog3.cfg]" \
#       -c "source [find target/infineon/cy8c6xx.cfg]" \
#       -c "init" \
#       -c "source D:/cy8ckit-prj/tools/flash_and_boot2.tcl"

halt
mww 0x40210080 0x05FA0003
echo "=== programming $HEX ==="
psoc6 sflash_restrictions 1
program $HEX verify
halt
echo "=== reset run ==="
reset run
sleep 500
shutdown
