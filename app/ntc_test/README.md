# app/ntc_test - NTC thermistor + DieTemp temperature demo (CY8CKIT-062-BLE + CY8CKIT-028-EPD)

Samples the **NCP18XH103F03RB NTC thermistor** (10 k at 25 C, B = 3380) on the
CY8CKIT-028-EPD shield plus the PSoC 6 **internal Die Temperature Sensor**,
and reports them over the UART.

Shield NTC circuit (A0..A3 = P10.0..P10.3):

```
A0 (P10.0) --0R-- VDD_REF        (full-scale reference rail - do NOT drive)
A0 -- 10k -- A1/A2 (P10.1/P10.2) -- NTC -- A3 (P10.3) --0R-- GND
```

The app reads all four pins as SAR ADC inputs (A0/A3 are the shield's power
rails and are never driven by the GPIO) plus the internal DTS, and prints the
**raw SAR counts**, the pin **millivolts** and best-effort temperatures:

```
A0=44512 A1=44640 A2=44768 A3=44864 DTS=450
mV: 1234 1245 1255 1263   NTC:  0.00 C   DTS:  21 C
```

- NTC: `Vsample = (A1+A2)/2 - A3`,
  `rNTC = R_ref * Vsample / (A0 - A3 - Vsample)`,
  `T = B / ln(rNTC / R_infinity) - 273.15` (verified against the Murata table).
- DTS: the factory-calibrated DieTemp conversion (VREF = VDDA, rescaled to the
  1.2 V bandgap basis).

The ADC follows **Infineon `mtb-example-hal-adc-basic`**: `cyhal_adc_init()` +
channel inits + `cyhal_adc_configure()` with `continuous_scanning=false`,
`vref=VDDA`, `vneg=VSSA`, 12-bit resolution and a 1 us acquisition time. The
millivolts use `cyhal_adc_read_uv()` (VBG-calibrated; the app_ext port falls
back to the raw-count scale when the VBG read is unavailable).

> **Note (hardware):** with the meter, both sides of the NTC measure ~1.2 V,
> so no voltage develops across the NTC and the computed NTC temperature is
> not meaningful right now. The raw counts / millivolts are the primary output.

## Build (offline)

```bash
./build.sh          # (Git Bash / modus-shell)
```

Produces `build/CY8CKIT-062-BLE/Debug/ntc_test.hex`.

`DEFINES=CY_USING_PREBUILT_CM0P_IMAGE` is required so `cybsp_init()` runs the
device config (100 MHz FLL) on the CM4.

## Flash + boot

Merge the CM4-only hex with the cat1cm0p (0x10000000) image (see
`tools/merge_hex.ps1`), then program + boot with `tools/flash_and_boot.tcl`.
UART: CYBSP_DEBUG_UART (P5_1 TX / P5_0 RX, 115200 8N1, COM26).
