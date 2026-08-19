# Retarget IO - STDIO Redirection for Embedded Applications

# Overview

A utility library that redirects standard input/output (`printf`/`scanf`) to a UART port or any custom I/O implementation, enabling STDIO console output from embedded firmware with a single initialization call.

# Features

- `printf()` and `scanf()` over UART with a single init call (HAL v3/MTB-HAL, HAL v2/CY-HAL, or PDL-only mode)
- Custom I/O backend to redirect STDIO to any interface such as USB CDC (`COMPONENT_RETARGET_IO_CUSTOM`)
- Thread-safe operation in RTOS environments (GCC/Newlib, ARM, and IAR toolchains)
- Dynamic baud rate change at runtime (MTB-HAL mode only)
- Selectable floating-point `printf` support to reduce flash when not needed

# When to Use

- When your embedded application needs console debug output via `printf()` over a UART or J-Link connection
- When integrating STDIO into an RTOS project that requires thread-safe console access
- When you need to redirect STDIO to a non-UART interface (e.g., USB CDC) without modifying application code
- When you need to dynamically change the UART baud rate at runtime

# How to Use

## Interface Selection

Select the backend for your project in your application `Makefile`:

| Mode | Condition | Notes |
|------|-----------|-------|
| HAL v3 (MTB-HAL) | `COMPONENT_MTB_HAL` set by device package | Requires a pre-initialized HAL UART object |
| HAL v2 (CY-HAL) | `CY_USING_HAL` set by device package | Library initializes UART from TX/RX pin arguments |
| PDL-only | Neither HAL component present | Requires a pre-initialized SCB UART; not thread-safe |
| Custom | `DEFINES += COMPONENT_RETARGET_IO_CUSTOM` in Makefile | Application provides `cy_retarget_io_getchar`/`putchar` |

**Note:** Only one interface mode can be active at a time. The HAL mode (HAL v3 or HAL v2) is determined automatically by the device package in your BSP and is not manually configurable.

### Optional Configuration Defines

Add these to the `DEFINES` variable in your Makefile as needed:

| Define | Effect |
|--------|--------|
| `CY_RETARGET_IO_CONVERT_LF_TO_CRLF` | Appends `\r` before each `\n` on STDOUT; recommended for full terminal compatibility |
| `CY_RETARGET_IO_NO_FLOAT` | Disables floating-point `printf` support to reduce flash usage |
| `CY_RTOS_AWARE` | Enables mutex-based thread safety for Newlib (GCC_ARM) in RTOS environments |

## Quick Start

### HAL v3 Mode (MTB-HAL)

1. Include the header at the top of your source file:
   ```c
   #include "cy_retarget_io.h"
   ```
2. Configure the UART peripheral in Device Configurator (name it `DEBUG_UART` to match the example below, and enable it). Then initialize the hardware using PDL and set up the HAL UART object:
   ```c
   Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, &DEBUG_UART_context);
   mtb_hal_uart_setup(&DEBUG_UART_hal_obj, &CYBSP_DEBUG_UART_hal_config,
                      &DEBUG_UART_context, NULL);
   Cy_SCB_UART_Enable(DEBUG_UART_HW);
   ```
3. Initialize retarget-io inside your startup/main function:
   ```c
   cy_retarget_io_init(&DEBUG_UART_hal_obj);
   ```
4. Use `printf()` normally.

**Optional – change baud rate at runtime:**
```c
uint32_t actual_baud = 0;
cy_retarget_io_change_baud_rate(9600, &actual_baud);
```
The host terminal must also be switched to the new baud rate.

**Optional – register a deepsleep callback** (only needed if your application enters deepsleep): the SCB clock stops during deepsleep, so without a callback any in-flight UART transmission is corrupted and the peripheral may be left in an undefined state on wakeup. The callback drains the TX FIFO before sleep entry and re-enables the UART on wakeup. In HAL v2 (CY-HAL) this is registered automatically; in HAL v3 (MTB-HAL) and PDL-only mode you must register it explicitly using `Cy_SysPm_RegisterCallback` with `mtb_syspm_scb_uart_deepsleep_callback` from the syspm-callbacks asset, or provide a custom callback. Refer to the PDL documentation.

### HAL v2 Mode (CY-HAL)

1. Include the header at the top of your source file:
   ```c
   #include "cy_retarget_io.h"
   ```
2. Initialize retarget-io inside your startup/main function:
   ```c
   cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
   ```
3. Use `printf()` normally.

**Note:** `CYBSP_DEBUG_UART_TX` and `CYBSP_DEBUG_UART_RX` are defined in the BSP. `CY_RETARGET_IO_BAUDRATE` defaults to 115200.

For flow control, use `cy_retarget_io_init_fc()` directly (see the Reference Guide section below).

### PDL-Only Mode

1. Include the header at the top of your source file:
   ```c
   #include "cy_retarget_io.h"
   ```
2. Configure the UART peripheral in Device Configurator (name it `DEBUG_UART` to match the example below, and enable it). Then initialize and enable it using PDL:
   ```c
   Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, NULL);
   Cy_SCB_UART_Enable(DEBUG_UART_HW);
   ```
3. Initialize retarget-io inside your startup/main function:
   ```c
   cy_retarget_io_init(DEBUG_UART_HW);
   ```
4. Use `printf()` normally.

**Note:** PDL-only mode is not thread-safe. Use `printf()` from a single thread or guard calls with a mutex.

### Custom I/O Mode

1. Add to your `Makefile`: `DEFINES += COMPONENT_RETARGET_IO_CUSTOM`
2. Implement `cy_retarget_io_getchar()` and `cy_retarget_io_putchar()` in your application (see the Reference Guide section below).
3. Call the appropriate `cy_retarget_io_init()` for your underlying mode (HAL v3, HAL v2, or PDL-only).
4. Use `printf()` normally.

**Example (USB CDC backend):**
```c
cy_rslt_t cy_retarget_io_getchar(char* c)
{
    return usb_cdc_read_char(c);
}

cy_rslt_t cy_retarget_io_putchar(char c)
{
    return usb_cdc_write_char(c);
}
```

**Note:** When using `COMPONENT_RETARGET_IO_CUSTOM`, a valid object/pointer must still be passed to `cy_retarget_io_init()` even though UART hardware is not used for I/O (in HAL modes this also initializes the RTOS mutex).

## Buffer Management

**Note:** The C standard library is not consistent in how it treats I/O streams. Many implementations buffer output by default and do not flush until the buffer is full — so if your `printf()` output does not appear, the data is likely sitting in an unflushed buffer rather than being sent to the UART. You must be aware of how your toolchain buffers data and choose an explicit flushing strategy. If you supply a buffer to `setvbuf()`, it must remain valid for the lifetime of the stream.

**Buffer size:** The default buffer size is defined by the `BUFSIZ` constant in `<stdio.h>`, which varies by compiler (typically 512–1024 bytes for embedded toolchains). This buffer is owned by the C standard library, not by retarget-io. The retarget-io library implements low-level I/O redirection (the `_write`/`_read` hooks that the standard library calls), so standard library buffering still applies on top of it unless explicitly disabled with `setvbuf()`.

To ensure immediate output, follow the guidance for your toolchain:

**GCC_ARM, LLVM, and ARM Compilers:**
These compilers support automatic buffer flushing control. You can disable buffering entirely:
```c
setvbuf(stdin, NULL, _IONBF, 0);   // Disable input buffering
setvbuf(stdout, NULL, _IONBF, 0);  // Disable output buffering
```

**IAR Compiler:**
IAR has limited standard library support in embedded projects. Buffer flushing requires explicit control:
- **Automatic flush:** End `printf` strings with `\n` (newline character)
- **Manual flush:** Call `fflush(stdout)` if available, or use newline characters

**Newlib-nano note:** Floating-point `%f` is unsupported by default. Add `-u _printf_float` to the linker command line to enable it.

**ISR note:** Do not call `printf()` from ISR context, especially when `CY_RTOS_AWARE` is defined.

## Line Ending Conversion

UART terminals typically require `\r\n` (CRLF) to move the cursor to the beginning of the next line. Using `\n` alone may cause staircase output on some terminals:

```
Line 1
      Line 2
            Line 3
```

To avoid this, define `CY_RETARGET_IO_CONVERT_LF_TO_CRLF` in your Makefile:

```makefile
DEFINES += CY_RETARGET_IO_CONVERT_LF_TO_CRLF
```

The library will then automatically prepend `\r` before each `\n` on STDOUT. No conversion occurs if `\r\n` is already present in the string. Alternatively, use `\r\n` explicitly in all `printf()` format strings.

## RTOS Integration

- Call `cy_retarget_io_init()` **after** the RTOS kernel starts.
- ARM and IAR compilers use mutexes automatically for thread-safe STDIO.
- Newlib (GCC_ARM): add `DEFINES += CY_RTOS_AWARE` to the Makefile to enable mutex protection in `_write()`.
- HAL support is required for RTOS use; PDL-only mode is not thread-safe.

# Reference Guide

For the full API documentation including all function signatures, parameters, and return values, see the [Reference Guide](https://infineon.github.io/retarget-io/html/index.html).

# Release Notes and Changelog

- RELEASE.md  - Detailed release notes for all versions

# License

This library is licensed under the **Apache License, Version 2.0**.

- [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)

---

### More information

* [Reference Guide](https://infineon.github.io/retarget-io/html/index.html)
* [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
* [Infineon GitHub](https://github.com/infineon)

# Copyright

© Copyright 2018-2026 Infineon Technologies AG and its affiliates. All rights reserved.
