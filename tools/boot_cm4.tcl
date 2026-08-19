# boot_cm4.tcl - manually boot the CM4 app directly from its vector table at 0x10020000
targets psoc6.cpu.cm4
halt
mww 0x40210080 0x05FA0003
set sp [mrw 0x10020000]
set pc [mrw 0x10020004]
echo "CM4 SP=0x$sp PC=0x$pc"
reg sp $sp
reg pc $pc
resume
echo "=== CM4 app started ==="
sleep 300
shutdown
