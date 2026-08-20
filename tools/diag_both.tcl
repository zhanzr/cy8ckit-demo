# diag_both.tcl - read CM0+ and CM4 PC/SP and fault status of a possibly-faulted boot
echo "=== target list ==="
targets
echo "=== CM4 ==="
targets psoc6.cpu.cm4
halt
set pcv [reg pc]
set lrv [reg lr]
set spv [reg sp]
set psr [reg xpsr]
puts "CM4 PC=$pcv"
puts "CM4 LR=$lrv"
puts "CM4 SP=$spv"
puts "CM4 xPSR=$psr"
echo "CM4 fault regs:"
set hfsr [read_memory 0xE000ED2C 32 1]
set cfsr [read_memory 0xE000ED28 32 1]
puts "CM4 HFSR=0x[format %08x $hfsr] CFSR=0x[format %08x $cfsr]"
echo "=== CM0+ ==="
targets psoc6.cpu.cm0
halt
set pcv0 [reg pc]
set spv0 [reg sp]
puts "CM0+ PC=$pcv0"
puts "CM0+ SP=$spv0"
resume
shutdown