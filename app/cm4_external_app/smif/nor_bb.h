#ifndef NOR_BB_H
#define NOR_BB_H

#include <stdint.h>

/* Initializes the GPIO bit-bang SPI interface to the S25FL512S
 * (SCK=P11.7, CS=P11.2, SI=P11.6, SO=P11.5). */
void nor_bb_init(void);

/* Erases the 256 KB sector containing `addr` and programs `len` bytes.
 * `len` must not cross a sector boundary. */
int nor_bb_program(uint32_t addr, const uint8_t *data, uint32_t len);

/* Reads `len` bytes starting at `addr`. */
int nor_bb_read(uint32_t addr, uint8_t *buf, uint32_t len);

#endif /* NOR_BB_H */
