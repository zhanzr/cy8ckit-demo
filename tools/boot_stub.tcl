# boot_stub.tcl - boot the app_ext XIP stub (releases CM4 to 0x18000000),
# then resume the CM4. Do not halt the CM4 first (it stays debug-free).
targets psoc6.cpu.cm0
halt
mww 0x40210080 0x05FA0003
set sp0 [mrw 0x10000000]
set pc0 [mrw 0x10000004]
echo "CM0+ SP=0x$sp0 PC=0x$pc0"
reg sp $sp0
reg pc $pc0
resume
sleep 500
targets psoc6.cpu.cm4
resume
sleep 300
shutdown
