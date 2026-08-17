#ifndef BUTTONS_CONFIG_H
#define BUTTONS_CONFIG_H

#include <Arduino.h>

/**
 * Button configuration for TRMNL-compatible devices.
 *
 * Each device defines its button layout via compile-time config.
 * PlatformIO build flags can override ADC thresholds for calibration.
 *
 * Buttons are categorized:
 *   - "action" buttons: call /api/action/[name] on press
 *   - "volume" buttons: navigate cached images locally (no API call)
 *
 * Range detection uses midpoint thresholds between adjacent buttons,
 * inspired by the papyrix-reader approach. A "no button" threshold
 * ensures idle ADC values don't trigger false presses.
 *
 * GPIO1 recorded averages: BACK=3512, CONF=2694, LEFT=1493, RIGHT=5
 * GPIO2 recorded averages: UP=2242, DOWN=5
 *
 * Midpoints (used as range boundaries):
 *   GPIO1: 3900 (idle), 3100, 2090, 750
 *   GPIO2: 3900 (idle), 1120
 */

typedef struct {
    const char *name;     // Button name used in API path: "back", "right", etc.
    int adc_min;          // Lower ADC threshold (exclusive)
    int adc_max;          // Upper ADC threshold (inclusive)
    int gpio;             // Which GPIO pin: 1 or 2
    bool is_action;       // true = calls API, false = local navigation only
} ButtonConfig;

// ============================================================================
// Xteink X4 — 6 buttons on 2 resistor-ladder ADC pins
// ============================================================================
#ifdef BOARD_XTEINK_X4

// GPIO1 action buttons — midpoint thresholds (can be overridden via build flags)
#ifndef X4_BTN_BACK_ADC_MIN
#define X4_BTN_BACK_ADC_MIN     3100
#endif
#ifndef X4_BTN_BACK_ADC_MAX
#define X4_BTN_BACK_ADC_MAX     3900
#endif
#ifndef X4_BTN_CONFIRM_ADC_MIN
#define X4_BTN_CONFIRM_ADC_MIN  2090
#endif
#ifndef X4_BTN_CONFIRM_ADC_MAX
#define X4_BTN_CONFIRM_ADC_MAX  3100
#endif
#ifndef X4_BTN_LEFT_ADC_MIN
#define X4_BTN_LEFT_ADC_MIN     750
#endif
#ifndef X4_BTN_LEFT_ADC_MAX
#define X4_BTN_LEFT_ADC_MAX     2090
#endif
#ifndef X4_BTN_RIGHT_ADC_MIN
#define X4_BTN_RIGHT_ADC_MIN    (-2147483647) // INT32_MIN — matches papyrix
#endif
#ifndef X4_BTN_RIGHT_ADC_MAX
#define X4_BTN_RIGHT_ADC_MAX    750
#endif

// GPIO2 volume buttons — midpoint thresholds (can be overridden via build flags)
#ifndef X4_BTN_VOLUP_ADC_MIN
#define X4_BTN_VOLUP_ADC_MIN    1120
#endif
#ifndef X4_BTN_VOLUP_ADC_MAX
#define X4_BTN_VOLUP_ADC_MAX    3900
#endif
#ifndef X4_BTN_VOLDN_ADC_MIN
#define X4_BTN_VOLDN_ADC_MIN    (-2147483647) // INT32_MIN — matches papyrix
#endif
#ifndef X4_BTN_VOLDN_ADC_MAX
#define X4_BTN_VOLDN_ADC_MAX    1120
#endif

// No-button threshold: ADC values above this mean nothing is pressed
#ifndef X4_ADC_NO_BUTTON
#define X4_ADC_NO_BUTTON        3900
#endif

// Number of buttons total
#define X4_BUTTON_COUNT 6

// Action button cooldown after press (ms) — prevents rapid repeated API calls
#ifndef X4_ACTION_COOLDOWN_MS
#define X4_ACTION_COOLDOWN_MS   500
#endif

static const ButtonConfig X4_BUTTONS[X4_BUTTON_COUNT] = {
    // Action buttons on GPIO1 (resistor ladder)
    {"back",    X4_BTN_BACK_ADC_MIN,    X4_BTN_BACK_ADC_MAX,    1, true},
    {"confirm", X4_BTN_CONFIRM_ADC_MIN, X4_BTN_CONFIRM_ADC_MAX, 1, true},
    {"left",    X4_BTN_LEFT_ADC_MIN,    X4_BTN_LEFT_ADC_MAX,    1, true},
    {"right",   X4_BTN_RIGHT_ADC_MIN,   X4_BTN_RIGHT_ADC_MAX,   1, true},
    // Volume buttons on GPIO2 (resistor ladder)
    {"volume_up",   X4_BTN_VOLUP_ADC_MIN, X4_BTN_VOLUP_ADC_MAX, 2, false},
    {"volume_down", X4_BTN_VOLDN_ADC_MIN, X4_BTN_VOLDN_ADC_MAX, 2, false},
};

/**
 * Build comma-separated list of action button names for X-Buttons header.
 * Only includes buttons where is_action == true.
 */
static inline String x4_action_buttons_list(void) {
    String list = "";
    for (int i = 0; i < X4_BUTTON_COUNT; i++) {
        if (X4_BUTTONS[i].is_action) {
            if (list.length() > 0) list += ",";
            list += X4_BUTTONS[i].name;
        }
    }
    return list;
}

#endif // BOARD_XTEINK_X4

#endif // BUTTONS_CONFIG_H