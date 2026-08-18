#ifndef SMIF_NOR_H
#define SMIF_NOR_H

/* Initializes the SMIF and the on-board S25FL512S SPI NOR (device mode,
 * no memory-mapped remapping). Called once by the CM0p before starting CM4. */
void smif_nor_init(void);

/* Runs the NOR erase/program/read benchmark and prints the report.
 * Must be called with the shared UART lock held (result block stays atomic).
 * The SMIF itself is protected by a cross-core spinlock inside. */
void smif_nor_benchmark(const char *core_label);

#endif /* SMIF_NOR_H */
