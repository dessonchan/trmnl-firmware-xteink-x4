#ifndef X4_BUTTONS_H
#define X4_BUTTONS_H

#include <Arduino.h>
#include "buttons_config.h"

#ifdef BOARD_XTEINK_X4

// Button enum for X4 side buttons
enum X4Button {
  X4_BTN_NONE = 0,
  X4_BTN_VOLUME_UP,    // GPIO2 — navigate previous (cached image)
  X4_BTN_VOLUME_DOWN,   // GPIO2 — navigate next (cached image)
  X4_BTN_RIGHT,         // GPIO1 — action button (API call)
  X4_BTN_LEFT,          // GPIO1 — action button (API call)
  X4_BTN_CONFIRM,       // GPIO1 — action button (API call)
  X4_BTN_BACK,          // GPIO1 — action button (API call)
};

// USB power detection: X4 uses GPIO20 (UART0_RXD) — HIGH when USB connected
#define X4_USB_DETECT_PIN         20

// GPIO pins for ADC button reading
#define X4_GPIO1_PIN              1
#define X4_GPIO2_PIN              2

// How long to poll for button presses after wakeup (ms)
#define X4_BUTTON_POLL_MS      2500
#define X4_BUTTON_POLL_INTERVAL_MS 50
// Debounce: require same reading for N consecutive samples
#define X4_BUTTON_DEBOUNCE_COUNT 3

// Awake loop settings (USB-powered mode)
#define X4_AWAKE_POLL_INTERVAL_MS 100   // How often to poll buttons and check USB (ms)
#define X4_BUTTON_COOLDOWN_MS     300   // Cooldown after volume button press (ms)

/**
 * @brief Check if USB power is connected on X4.
 *        Uses GPIO20 (UART0_RXD) which reads HIGH when USB is connected.
 * @return true if USB connected, false otherwise
 */
bool x4_is_usb_connected(void);

/**
 * @brief Initialize ADC pins for X4 button reading
 */
void x4_buttons_init(void);

/**
 * @brief Read currently pressed button (single ADC sample)
 * @return The button currently being pressed, or X4_BTN_NONE
 */
X4Button x4_read_button(void);

/**
 * @brief Poll buttons for X4_BUTTON_POLL_MS after wakeup.
 *        Returns the first detected navigation button (Volume Up/Down),
 *        or X4_BTN_NONE if no relevant button was pressed.
 * @return X4_BTN_VOLUME_UP, X4_BTN_VOLUME_DOWN, or X4_BTN_NONE
 */
X4Button x4_poll_buttons_after_wakeup(void);

/**
 * @brief Enter awake loop while USB power is connected.
 *        Polls ADC buttons for navigation and checks USB status.
 *        Exits when USB is disconnected.
 *        Note: actual loop logic is in bl.cpp (x4_awake_loop_impl)
 */
void x4_awake_loop_enter(void);

/**
 * @brief Exit awake loop — clears the in-loop flag.
 */
void x4_awake_loop_exit(void);

/**
 * @brief Check if currently in X4 awake loop (USB powered, not sleeping).
 *        Used by goToSleep() to skip deep sleep and stay awake instead.
 * @return true if in awake loop, false otherwise
 */
bool x4_is_in_awake_loop(void);

/**
 * @brief Check if a button is an "action" button (calls API on press).
 * @param btn The button to check
 * @return true if this button should trigger an API call
 */
bool x4_is_action_button(X4Button btn);

/**
 * @brief Get the API action name for a button.
 *        Returns the name used in the URL path: /api/action/<name>
 * @param btn The button to query
 * @return Action name string (e.g. "back", "right"), or nullptr if not an action button
 */
const char* x4_get_action_name(X4Button btn);

/**
 * @brief Get the comma-separated list of available action button names.
 *        Used for the X-Buttons HTTP header.
 * @return Comma-separated string, e.g. "back,right,left,confirm"
 */
String x4_action_buttons_list(void);

/**
 * @brief Read an ADC sample from a specific GPIO pin.
 * @param gpio The GPIO pin number (1 or 2)
 * @return Raw 12-bit ADC value (0-4095)
 */
int x4_read_adc(int gpio);

#endif // BOARD_XTEINK_X4

#endif // X4_BUTTONS_H