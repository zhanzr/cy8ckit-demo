# Thermistor (Temperature Sensor) Release Notes

This library provides functions for reading a thermistor temperature sensor as used on some boards and shields.

### What's Included?
* APIs for initializing/de-initializing the driver
* APIs for reading temperature

### What Changed?
#### v2.1.0
* Updated call to ADC init to avoid deprecated function call
* Added support for HAL API v1 or v2
#### v2.0.0
* mtb_thermistor_ntc_gpio_init signature changed to add wiring.
cy_rslt_t mtb_thermistor_ntc_gpio_init(mtb_thermistor_ntc_gpio_t* obj, cyhal_adc_t* adc,
                                        cyhal_gpio_t gnd, cyhal_gpio_t vdd, cyhal_gpio_t out,
                                        mtb_thermistor_ntc_gpio_cfg_t* cfg, mtb_thermistor_ntc_wiring wiring)
#### v1.0.0
* Initial release

### Supported Software and Tools
This version of the thermistor library was validated for compatibility with the following Software and Tools:

| Software and Tools                        | Version |
| :---                                      | :----:  |
| ModusToolbox™ Software Environment        | 2.4.0   |
| GCC Compiler                              | 10.3.1  |
| IAR Compiler                              | 8.4     |
| ARM Compiler 6                            | 6.11    |

Minimum required ModusToolbox™ Software Environment: v2.0

### More information

* [API Reference Guide](https://infineon.github.io/thermistor/html/index.html)
* [Cypress Semiconductor, an Infineon Technologies Company](http://www.cypress.com)
* [Infineon GitHub](https://github.com/infineon)
* [ModusToolbox™](https://www.cypress.com/products/modustoolbox-software-environment)
* [PSoC™ 6 Code Examples using ModusToolbox™ IDE](https://github.com/infineon/Code-Examples-for-ModusToolbox-Software)
* [ModusToolbox™ Software](https://github.com/Infineon/modustoolbox-software)
* [PSoC™ 6 Resources - KBA223067](https://community.cypress.com/docs/DOC-14644)

---
© Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation, 2019-2021.
