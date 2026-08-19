# Thermistor (Temperature Sensor)

### Overview

This library provides functions for reading a thermistor temperature sensor as used on some shields.

### Quick Start
1. Create an empty PSoC™ 6 application
2. Add this library to the application
3. Add retarget-io library using Library Manager
4. Place following code in the main.c file:
5. Note: CY8CKIT_028_EPD_ items are all defined in the CY8CKIT-028-EPD library. If you are using different hardware, pick appropriate values to substitute for your hardware.
```cpp
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cy8ckit_028_epd.h"

cyhal_adc_t adc;
mtb_thermistor_ntc_gpio_t thermistor;
mtb_thermistor_ntc_gpio_cfg_t thermistor_cfg = {
    .r_ref = CY8CKIT_028_EPD_THERM_R_REF,
    .b_const = CY8CKIT_028_EPD_THERM_B_CONST,
    .r_infinity = CY8CKIT_028_EPD_THERM_R_INFINITY,
};

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* Intialize adc */
    result = cyhal_adc_init(&adc, CY8CKIT_028_EPD_PIN_THERM_OUT1, NULL);

    /* Initialize thermistor */
    result = mtb_thermistor_ntc_gpio_init(&thermistor, &adc,
        CY8CKIT_028_EPD_PIN_THERM_GND, CY8CKIT_028_EPD_PIN_THERM_VDD, CY8CKIT_028_EPD_PIN_THERM_OUT1,
        &thermistor_cfg, MTB_THERMISTOR_NTC_WIRING_VIN_R_NTC_GND);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    for (;;)
    {
        /* Measure the temperature and send the value via UART */
        printf("Temperature = %fC\n\r", mtb_thermistor_ntc_gpio_get_temp(&thermistor));
        cyhal_system_delay_ms(1000);
    }
}
```
6. Build the application and program the kit.

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
