#ifndef X4_BUTTONS_H
#define X4_BUTTONS_H

#include <Arduino.h>

#ifdef BOARD_XTEINK_X4

// Button enum for Xteink X4 side buttons
enum X4Button {
  X4_BTN_NONE = 0,
  X4_BTN_VOLUME_UP,    // GPIO2 ≈ 2205 — previous screen
  X4_BTN_VOLUME_DOWN,   // GPIO2 ≈ 3   — next screen
  // GPIO1 buttons available for future use
  X4_BTN_RIGHT,         // GPIO1 ≈ 3
  X4_BTN_LEFT,          // GPIO1 ≈ 1470
  X4_BTN_CONFIRM,      // GPIO1 ≈ 2655
  X4_BTN_BACK,         // GPIO1 ≈ 3470
};

// ADC threshold values (with tolerance)
// GPIO1 (4 buttons on resistor ladder)
#define X4_GPIO1_PIN           1
#define X4_BTN_RIGHT_THRESH    3
#define X4_BTN_LEFT_THRESH     1470
#define X4_BTN_CONFIRM_THRESH  2655
#define X4_BTN_BACK_THRESH     3470

// GPIO2 (2 buttons on resistor ladder)
#define X4_GPIO2_PIN            2
#define X4_BTN_VOLDN_THRESH    3
#define X4_BTN_VOLUP_THRESH    2205

// Tolerance for ADC threshold comparison
#define X4_ADC_TOLERANCE       100

// How long to poll for button presses after wakeup (ms)
#define X4_BUTTON_POLL_MS      2500
#define X4_BUTTON_POLL_INTERVAL_MS 50
// Debounce: require same reading for N consecutive samples
#define X4_BUTTON_DEBOUNCE_COUNT 3

// Awake loop settings (USB-powered mode)
#define X4_AWAKE_POLL_INTERVAL_MS 100   // How often to poll buttons and check USB (ms)
#define X4_BUTTON_COOLDOWN_MS     300   // Cooldown after a button press (ms)

// USB power detection: X4 uses GPIO20 (UART0_RXD) — HIGH when USB connected
#define X4_USB_DETECT_PIN         20

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
 * @brief Read currently pressed button (single sample)
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
 *        Exits when USB is disconnected or refresh interval elapses.
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

#endif // BOARD_XTEINK_X4

#endif // X4_BUTTONS_H