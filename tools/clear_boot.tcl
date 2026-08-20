# clear_boot.tcl - clear stale CM0+ SRAM residue, then boot the XIP stub
# (the stub inits SMIF and releases the CM4 to 0x18000000 itself)
halt
targets psoc6.cpu.cm0
halt
echo "=== clear CM0+ SRAM 0x08000000..0x08008000 ==="
for { set a 0x08000000 } { $a < 0x08008000 } { incr a 4 } {
  mww $a 0x00000000
}
echo "cleared"
mww 0x40210080 0x05FA0003
set sp0 [mrw 0x10000000]
set pc0 [mrw 0x10000004]
echo "CM0+ SP=0x$sp0 PC=0x$pc0"
reg sp $sp0
reg pc $pc0
resume
sleep 300
shutdown
