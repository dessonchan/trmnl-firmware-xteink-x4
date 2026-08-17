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

int x4_read_adc(int gpio)
{
  return analogRead(gpio);
}

X4Button x4_read_button(void)
{
  int adc1 = analogRead(X4_GPIO1_PIN);
  int adc2 = analogRead(X4_GPIO2_PIN);

  // Discard first ADC readings to flush sample-and-hold charge
  // (battery monitor on GPIO0 shares ADC1 — same technique as papyrix-reader)
  (void)analogRead(X4_GPIO1_PIN);
  (void)analogRead(X4_GPIO2_PIN);
  adc1 = analogRead(X4_GPIO1_PIN);
  adc2 = analogRead(X4_GPIO2_PIN);

  // Check for idle state first — ADC above threshold means no button pressed
  if (adc1 > X4_ADC_NO_BUTTON && adc2 > X4_ADC_NO_BUTTON)
  {
    return X4_BTN_NONE;
  }

  // Iterate through the button config table to find a match.
  // Range check: adc_min < adc <= adc_max (exclusive lower, inclusive upper)
  for (int i = 0; i < X4_BUTTON_COUNT; i++)
  {
    const ButtonConfig &cfg = X4_BUTTONS[i];
    int adc = (cfg.gpio == X4_GPIO1_PIN) ? adc1 : adc2;

    if (adc > cfg.adc_min && adc <= cfg.adc_max)
    {
      // Map config name back to X4Button enum
      if (strcmp(cfg.name, "back") == 0)        return X4_BTN_BACK;
      if (strcmp(cfg.name, "right") == 0)       return X4_BTN_RIGHT;
      if (strcmp(cfg.name, "confirm") == 0)    return X4_BTN_CONFIRM;
      if (strcmp(cfg.name, "left") == 0)        return X4_BTN_LEFT;
      if (strcmp(cfg.name, "volume_up") == 0)   return X4_BTN_VOLUME_UP;
      if (strcmp(cfg.name, "volume_down") == 0)  return X4_BTN_VOLUME_DOWN;
    }
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
          else if (current == X4_BTN_RIGHT) name = "Right";
          else if (current == X4_BTN_LEFT) name = "Left";
          else if (current == X4_BTN_CONFIRM) name = "Confirm";
          else if (current == X4_BTN_BACK) name = "Back";
          Log_info("X4 button: %s pressed", name);

          // Return any navigation or action button (not just Volume)
          if (current != X4_BTN_NONE)
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

bool x4_is_action_button(X4Button btn)
{
  switch (btn)
  {
  case X4_BTN_BACK:
  case X4_BTN_RIGHT:
  case X4_BTN_LEFT:
  case X4_BTN_CONFIRM:
    return true;
  default:
    return false;
  }
}

const char* x4_get_action_name(X4Button btn)
{
  switch (btn)
  {
  case X4_BTN_BACK:    return "back";
  case X4_BTN_RIGHT:   return "right";
  case X4_BTN_LEFT:    return "left";
  case X4_BTN_CONFIRM: return "confirm";
  default:             return nullptr;
  }
}

#endif // BOARD_XTEINK_X4