#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p .build/battery-tests
clang -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter \
    -DBRIDGE75_BATTERY_MONITOR \
    -Ibattery-project/tests/stubs -Ikeyboards/shortcut/bridge75 -Ikeyboards/shortcut/wireless \
    -fsanitize=undefined,bounds -g \
    battery-project/tests/test_battery.c \
    keyboards/shortcut/bridge75/battery_monitor.c keyboards/shortcut/wireless/module.c \
    -o .build/battery-tests/test-battery
.build/battery-tests/test-battery
clang -std=c11 -Wall -Wextra -Werror \
    '-DQMK_KEYBOARD_H="quantum.h"' \
    -Ibattery-project/tests/lighting_stubs -Ikeyboards/shortcut/bridge75 \
    -fsanitize=undefined,bounds -g \
    battery-project/tests/test_lighting.c keyboards/shortcut/bridge75/battery_monitor.c \
    keyboards/shortcut/bridge75/ansi/keymaps/battery/battery_lighting.c \
    -o .build/battery-tests/test-lighting
.build/battery-tests/test-lighting
