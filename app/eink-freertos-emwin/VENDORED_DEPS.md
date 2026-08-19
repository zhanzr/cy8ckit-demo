# Vendored dependencies (app/mtb_shared)

The ModusToolbox dependencies of this project are vendored into this
repository at the workspace-level `app/mtb_shared/<name>/<version>` (no
`.git` directories), shared with `app/eink_test`.

For reproducibility, the exact version and source git hash (where the asset is
a git clone) are listed below. Git-hash-less assets (released as tarballs by
Infineon) are identified by their version.

| Asset | Version | Source commit |
|-------|---------|---------------|
| cat1cm0p | release-v1.11.0 | (tarball release) |
| cmsis | release-v6.1.0 | (tarball release) |
| core-lib | release-v1.8.0 | (tarball release) |
| core-make | release-v3.9.0 | (tarball release) |
| emwin | release-v6.32.0 | 5728609669184333423f72e9ac8a16a830350fe9 |
| freertos | latest-v10.X | 8a19c8db81becf1e981a5f94630952160fddf8c5 |
| mtb-hal-cat1 | release-v2.7.4 | (tarball release) |
| mtb-pdl-cat1 | release-v3.23.0 | (tarball release) |
| recipe-make-cat1a | release-v2.8.0 | (tarball release) |
| retarget-io | latest-v1.X | 83bbf77931f809c243be20d58939c52c674674f7 |
| display-eink-e2271cs021 | release-v1.1.0 | f6faa7d18efc4b8f0ec1ed7935c73171f4acdf95 |
| CY8CKIT-028-EPD | latest-v2.X | 753afb355486eb1687cf91dec5d8d2728145ab6c |
| abstraction-rtos | release-v1.13.0 | de63f516d822cde10ccc0ab408ffafe7a13db8f7 |
| BMI160_driver | bmi160_v3.9.1 | 3ac7bb6a23797d637de2d8e7542b60eb1fe688e5 |
| clib-support | release-v1.9.0 | 2a14bd0a1470d580a2c98f140551ea8cdc7f8f59 |
| sensor-motion-bmi160 | release-v1.1.2 | a3c5034df153c2109b7c83347766a342ee78f89a |
| thermistor | release-v2.1.0 | 25d0d345d3dbc59dbda07c677d1261ff304b96f4 |

All assets are from the Infineon/Cypress ModusToolbox asset repo
(https://github.com/Infineon / https://github.com/cypresssemiconductorco).
