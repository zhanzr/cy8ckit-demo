# app/ntc_test - NTC thermistor + DieTemp temperature demo (CY8CKIT-062-BLE + CY8CKIT-028-EPD)

Samples the **NCP18XH103F03RB NTC thermistor** (10 k at 25 C, B = 3380) on the
CY8CKIT-028-EPD shield plus the PSoC 6 **internal Die Temperature Sensor**,
and reports them over the UART.

Shield NTC circuit (A0..A3 = P10.0..P10.3):

```
A0 (P10.0) -- 10k -- A1/A2 (P10.1/P10.2) -- NTC -- A3 (P10.3)
```

The 0R strap resistors that would tie A0 to VIO_REF and A3 to GND are **not
populated** on this shield, so the GPIO drives the divider: **A0 is driven
high** (VDDD ~3.3 V supply) and **A3 low** (0 V return). Only **A1/A2** (the
divider output) are sampled by the SAR ADC; the driven A0/A3 pins cannot be
read cleanly through the SARMUX. The app prints the raw counts, the pin mV and
the temperatures:

```
A1=47136 A2=47136 DTS=1200
mV: 1448 1448   NTC:  31.62 C   DTS:   33 C
```

- NTC: `OUT = (A1+A2)/2`, `rNTC = R_ref * OUT / (VDD - OUT)`,
  `T = B / ln(rNTC / R_infinity) - 273.15` (verified against the Murata table).
- DTS: the 1.2 V bandgap reference + the factory calibration (the same as
  `app_ext/blink_hello`), with the SAR aperture sized to ~1.15 us from the
  actual SAR clock (a longer aperture droops the DieTemp count).

The ADC follows **Infineon `mtb-example-hal-adc-basic`**: `cyhal_adc_init()` +
channel inits + `cyhal_adc_configure()` with `continuous_scanning=false`,
`vref=VDDA`, `vneg=VSSA`, 12-bit resolution and a 1 us acquisition time. The
millivolts use `cyhal_adc_read_uv()` (VBG-calibrated) with a raw-count
fallback for the XIP port where the VBG read is unavailable.

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
