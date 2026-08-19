# boot_xip.tcl - boot the app_ext CM0+ XIP stub (which boots CM4 from NOR);
# explicitly reset the CM4 so a previously-running app is replaced.
mww 0x40210080 0x05FA0003
targets psoc6.cpu.cm0
halt
set sp0 [mrw 0x10000000]
set pc0 [mrw 0x10000004]
echo "CM0+ SP=0x$sp0 PC=0x$pc0"
reg sp $sp0
reg pc $pc0
resume
# Reset + release the CM4 so it re-fetches its vector from the external NOR.
targets psoc6.cpu.cm4
psoc6.cpu.cm4 arp_reset assert 0
resume
sleep 300
shutdown
