/* Linker script for the S25FL512S probe-rs flash algorithm.
 *
 * Everything (code + literal pools) is placed in one loadable section at
 * 0x08000000 (PSoC6 SRAM) so the extracted binary blob is fully
 * position-independent when probe-rs loads it to its chosen RAM address.
 */

MEMORY
{
  RAM (rwx) : ORIGIN = 0x08000000, LENGTH = 0x4000
}

SECTIONS
{
  .text :
  {
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
  } > RAM

  /DISCARD/ :
  {
    *(.ARM.exidx*)
    *(.ARM.attributes*)
    *(.comment*)
    *(.note*)
    *(.debug*)
  }
}
