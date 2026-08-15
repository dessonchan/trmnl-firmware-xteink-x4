#ifdef BOARD_XTEINK_X4

#include "x4_buttons.h"
#include <Arduino.h>
#include "trmnl_log.h"

void x4_buttons_init(void)
{
  // Suppress ESP-IDF GPIO driver info logs (analogRead reconfigures pins each call)
  esp_log_level_set("gpio", ESP_LOG_ERROR);

  pinMode(X4_GPIO1_PIN, INPUT);
  pinMode(X4_GPIO2_PIN, INPUT);
  // Configure ADC resolution for 12-bit (0-4095)
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // Full range: ~0-3.6V
  // Initialize USB detection pin
  pinMode(X4_USB_DETECT_PIN, INPUT);
  Log_info("X4 buttons: ADC initialized (GPIO1=%d, GPIO2=%d, USB detect=%d)", X4_GPIO1_PIN, X4_GPIO2_PIN, X4_USB_DETECT_PIN);
}

bool x4_is_usb_connected(void)
{
  return digitalRead(X4_USB_DETECT_PIN) == HIGH;
}

X4Button x4_read_button(void)
{
  int btn1 = analogRead(X4_GPIO1_PIN);
  int btn2 = analogRead(X4_GPIO2_PIN);

  // Check GPIO2 first (Volume Up/Down — our primary navigation buttons)
  // Volume Down has the lowest value (~3), so check it first
  if (btn2 < X4_BTN_VOLDN_THRESH + X4_ADC_TOLERANCE)
  {
    return X4_BTN_VOLUME_DOWN;
  }
  // Volume Up (~2205) — use simple < check (matches sample code logic)
  if (btn2 < X4_BTN_VOLUP_THRESH + X4_ADC_TOLERANCE)
  {
    return X4_BTN_VOLUME_UP;
  }

  // Check GPIO1 (4 buttons on resistor ladder)
  // Right has the lowest value (~3)
  if (btn1 < X4_BTN_RIGHT_THRESH + X4_ADC_TOLERANCE)
  {
    return X4_BTN_RIGHT;
  }
  // Left (~1470)
  if (btn1 < X4_BTN_LEFT_THRESH + X4_ADC_TOLERANCE)
  {
    return X4_BTN_LEFT;
  }
  // Confirm (~2655)
  if (btn1 < X4_BTN_CONFIRM_THRESH + X4_ADC_TOLERANCE)
  {
    return X4_BTN_CONFIRM;
  }
  // Back (~3470)
  if (btn1 < X4_BTN_BACK_THRESH + X4_ADC_TOLERANCE)
  {
    return X4_BTN_BACK;
  }

  return X4_BTN_NONE;
}

X4Button x4_poll_buttons_after_wakeup(void)
{
  X4Button last_button = X4_BTN_NONE;
  int debounce_count = 0;
  unsigned long poll_start = millis();
  unsigned long quick_check_end = poll_start + 200;

  while (millis() - poll_start < X4_BUTTON_POLL_MS)
  {
    X4Button current = x4_read_button();

    if (current != X4_BTN_NONE)
    {
      if (current == last_button)
      {
        debounce_count++;
        if (debounce_count >= X4_BUTTON_DEBOUNCE_COUNT)
        {
          const char *name = "Unknown";
          if (current == X4_BTN_VOLUME_UP) name = "Volume Up";
          else if (current == X4_BTN_VOLUME_DOWN) name = "Volume Down";
          Log_info("X4 button: %s pressed", name);

          if (current == X4_BTN_VOLUME_UP || current == X4_BTN_VOLUME_DOWN)
          {
            return current;
          }
        }
      }
      else
      {
        last_button = current;
        debounce_count = 1;
      }
    }
    else
    {
      last_button = X4_BTN_NONE;
      debounce_count = 0;
    }

    // Early exit: if past quick-check window and no button seen at all
    if (millis() > quick_check_end && last_button == X4_BTN_NONE && debounce_count == 0)
    {
      return X4_BTN_NONE;
    }

    delay(X4_BUTTON_POLL_INTERVAL_MS);
  }

  return X4_BTN_NONE;
}

// Flag to prevent goToSleep() from entering deep sleep while in awake loop
static bool x4_in_awake_loop = false;

bool x4_is_in_awake_loop(void)
{
  return x4_in_awake_loop;
}

void x4_awake_loop_enter(void)
{
  x4_in_awake_loop = true;
}

void x4_awake_loop_exit(void)
{
  x4_in_awake_loop = false;
}
#endif // BOARD_XTEINK_X4