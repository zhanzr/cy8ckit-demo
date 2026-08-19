# reset_halt.tcl - reset both cores into a clean halted state (SMIF not in XIP)
reset halt
targets psoc6.cpu.cm0
halt
targets psoc6.cpu.cm4
halt
echo "=== both cores halted ==="
shutdown
