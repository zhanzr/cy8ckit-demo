# boot_pc.tcl - boot the CM0+ image (0x10000000) + CM4, then report the CM4 PC
# and fault status a few seconds in (checks the app is alive, not hard-faulted).
targets psoc6.cpu.cm0
halt
mww 0x40210080 0x05FA0003
set sp0 [mrw 0x10000000]
set pc0 [mrw 0x10000004]
echo "CM0+ SP=0x$sp0 PC=0x$pc0"
reg sp $sp0
reg pc $pc0
resume
sleep 2500
targets psoc6.cpu.cm4
halt
echo "CM4 PC after boot = [reg pc]"
echo "CFSR [mrw 0xE000ED28] HFSR [mrw 0xE000ED2C]"
resume
sleep 3000
targets psoc6.cpu.cm4
halt
echo "CM4 PC @5s = [reg pc]"
shutdown
