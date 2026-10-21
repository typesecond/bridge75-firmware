// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Fn controls the gauge lifetime. The general RGB inactivity timer would
// otherwise hide it after a long hold, even while its LED rail remains on.
// Suspend and wireless low-power states still disable the gauge.
#undef RGB_MATRIX_TIMEOUT
#define RGB_MATRIX_TIMEOUT 0
