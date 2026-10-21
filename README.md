# Bridge75 Fn battery gauge

Use the Bridge75's function-key lights as a battery indicator. Hold **Fn** to
show the wireless module's reported charge across **F1–F12**; release Fn to
turn the lights off. Both default Windows and Mac Fn layers work.

- **50%:** F1–F6 light up. **75%:** F1–F9. **100%:** all twelve.
- Colors run red → orange → yellow → green. The last segment is proportional.
- A real 0% flashes F1 red. Unknown or stale readings show dim purple.
- Other keys stay dark, including Caps Lock. RGB shortcuts are suppressed.
- Normal key layouts, wireless controls, and VIA support are retained.

**Status:** software-validated candidate for **Bridge75 ANSI RGB / Max**.
The full firmware compiles, and the native tests pass. This candidate has not
been validated on hardware. Battery accuracy and precision depend on the
wireless module's reports.

## Source layout

This repository is a source overlay for [emolitor/qmk_firmware](https://github.com/emolitor/qmk_firmware)
**em-2.0.0**, commit `1153ac2b39e937a2c2f6bd1e956643518a2117e3`.
It includes every changed or added firmware file; the build script applies
them to the pinned upstream checkout. It does not contain an entire QMK fork.

- `keyboards/`: battery telemetry, rendering, and dedicated `battery` keymap.
- `battery-project/tests/`: native tests with GPIO, UART, and RGB stubs.
- `firmware/`: compiled candidate, build identity, and SHA-256 checksum.
- `scripts/build.sh`: prepare upstream, apply the overlay, and compile.

## Test

Requires Clang:

```sh
sh battery-project/test.sh
```

Tests exercise the actual UART parser and lighting hooks with mocked hardware:
all 101 battery levels, checksum/range validation, freshness, fragmentation,
timer rollover, Win/Mac Fn, release, suspend/wake, previous RGB colors,
render chunks, and missing/remapped LEDs. Undefined-behavior and bounds
sanitizers are enabled. These tests do not emulate the entire keyboard.

## Build

Install QMK's build dependencies and an Arm bare-metal GNU toolchain. The
checked-in binary was built with Arm GNU 14.2.Rel1 (GCC 14.2.1).
With `qmk` available in the active environment:

```sh
sh scripts/build.sh
# Or select a compiler prefix explicitly:
ARM_PREFIX=/path/to/bin/arm-none-eabi- sh scripts/build.sh
```

The script checks out the pinned QMK revision under `.build/qmk`, initializes
its submodules, copies this overlay, and builds
`shortcut/bridge75/ansi:battery`. The result is
`.build/qmk/shortcut_bridge75_ansi_battery.bin`. It does not flash a device.

To use an existing checkout instead, check out the exact base commit, initialize
its submodules, copy this repository's `keyboards/` contents into `keyboards/`,
and run `qmk compile -kb shortcut/bridge75/ansi -km battery`.

## Candidate binary

[`firmware/bridge75-ansi-fn-battery.bin`](firmware/bridge75-ansi-fn-battery.bin)
is 49,996 bytes. Verify it from the firmware directory:

```sh
cd firmware
shasum -a 256 -c SHA256SUMS
```

The gauge uses checksum-verified module packets, rejects percentages above 100,
and marks readings stale after 15 seconds. Source/connection transitions and
sleep/wake invalidate old readings. It queries in USB mode as well as wireless.
The lighting policy uses temporary, non-EEPROM RGB settings and clears each
render chunk so saved effect flags cannot leave ambient colors behind.
Fn determines gauge visibility; suspend and wireless low-power states override it.

## License

GPL-2.0-or-later for the battery implementation. Upstream files retain their
original copyright and license notices. See [LICENSE](LICENSE) and the
[upstream licensing information](https://github.com/emolitor/qmk_firmware/blob/em-2.0.0/license_GPLv2.md).
