/***************************************************************************//**
* Linker script for the CM4 app that is stored in the external SPI NOR flash
* (0x18000000) and booted by the CM0p after copying it into internal SRAM.
*
* Everything (vectors + code + data) is placed at 0x08030000 with LMA == VMA,
* so the startup copy table is a self-copy (no-op) and the image is fully
* self-contained once the raw binary is loaded at 0x08030000.
*******************************************************************************/

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)
ENTRY(Reset_Handler)

STACK_SIZE = 0x2000;

MEMORY
{
    /* The whole image lives in internal SRAM at 0x08030000 (above the CM0p
     * RAM 0x08000000-0x08005000 and the shared region up to 0x08005100). */
    ram               (rwx)   : ORIGIN = 0x08030000, LENGTH = 0x10000
}

SECTIONS
{
    .cy_app_header :
    {
        KEEP(*(.cy_app_header))
    } > ram

    .text :
    {
        . = ALIGN(4);
        __Vectors = . ;
        KEEP(*(.vectors))
        . = ALIGN(4);
        __Vectors_End = .;
        __Vectors_Size = __Vectors_End - __Vectors;

        . = ALIGN(4);
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *(.rodata .rodata.* .constdata .constdata.* .conststring .conststring.*)
        KEEP(*(.eh_frame*))
    } > ram

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ram

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ram
    __exidx_end = .;

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        /* Keep the vector "copy" a self-copy: the image runs in place at
         * 0x08030000, so the CPU vector table stays at __Vectors. */
        LONG (__Vectors)                                    /* From */
        LONG (__ram_vectors_start__)                        /* To   */
        LONG (__Vectors_End - __Vectors)                    /* Size */
        LONG (__etext)                                      /* From */
        LONG (__data_start__)                               /* To   */
        LONG (__data_end__ - __data_start__)                /* Size */
        __copy_table_end__ = .;
    } > ram

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (__bss_start__)
        LONG (__bss_end__ - __bss_start__)
        __zero_table_end__ = .;
    } > ram

    __etext = .;

    /* The vector table is used in place at __Vectors (0x08030000); there is
     * no separate RAM vector copy. */
    __ram_vectors_start__ = __Vectors;
    __ram_vectors_end__   = __Vectors_End;

    .data :
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(.data*)

        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;

        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;

        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;

        KEEP(*(.jcr*))
        . = ALIGN(4);
        __data_end__ = .;
    } > ram

    .noinit (NOLOAD) : ALIGN(8)
    {
        KEEP(*(.noinit))
    } > ram

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > ram

    .heap (NOLOAD):
    {
        __HeapBase = .;
        __end__ = .;
        end = __end__;
        KEEP(*(.heap*))
        . = ORIGIN(ram) + LENGTH(ram) - STACK_SIZE;
        __HeapLimit = .;
    } > ram

    .stack_dummy (NOLOAD):
    {
        KEEP(*(.stack*))
    } > ram

    __StackTop = ORIGIN(ram) + LENGTH(ram);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}

__cy_memory_0_start    = 0x08030000;
__cy_memory_0_length   = 0x10000;
__cy_memory_0_row_size = 0x200;
