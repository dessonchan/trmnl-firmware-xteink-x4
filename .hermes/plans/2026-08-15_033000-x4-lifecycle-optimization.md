# X4 Lifecycle Optimization Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Optimize the xteink X4 firmware lifecycle for faster cycles, lower memory pressure, and less redundant work — without affecting other board targets.

**Architecture:** All changes are guarded by `#ifdef BOARD_XTEINK_X4` so other boards are unaffected. The optimizations target four areas: WiFi reconnect elimination in awake loop, reduced blocking time on button-wake, SPIFFS/filesystem overhead, and logging overhead.

**Tech Stack:** ESP32-C3 (80MHz, 320KB heap, no PSRAM), PlatformIO, Arduino framework, SPIFFS, WiFiClientSecure

---

## Context

### Current lifecycle (from analysis)

Typical timer-wake cycle: **~10-12s awake time**
- Serial init: 200ms-2s (DEV_FIRMWARE waits for serial)
- Button polling on GPIO wake: 2.5s blocking (always, even if no button pressed)
- WiFi connect: 1.5s (fast) to 45s (worst case)
- NTP sync: 2-5s
- submitStoredLogs: called **twice** (before + after downloadAndShow)
- API display request: 1-30s
- Image download + SPIFFS write + purge: 0.5-5s
- Image decode + display: 2-5s
- Display sleep + goToSleep: 100ms

### Awake loop (USB-powered)
- Polls ADC buttons every 100ms
- On scheduled refresh: reconnects WiFi from scratch (downloadAndShow internally disconnects WiFi), re-runs full downloadAndShow
- No mDNS cleanup between reconnects

### Key bottlenecks identified
1. `downloadAndShow()` calls `WiFi.disconnect(true)` after download → awake loop must full-reconnect
2. `x4_poll_buttons_after_wakeup()` always blocks 2.5s even if no button is pressed
3. `submitStoredLogs()` called twice per cycle
4. `list_files()` iterates all SPIFFS files on every boot
5. `filesystem_file_exists()` logs on every check
6. `display_read_file()` uses `Serial.println` directly (always active)
7. `heap_caps_check_integrity_all(true)` called during image download
8. Image download uses byte-by-byte `stream->read()`

---

## Tasks

### Task 1: Skip WiFi disconnect in awake loop

**Objective:** Prevent `downloadAndShow()` from disconnecting WiFi when in the X4 awake loop, eliminating the full WiFi reconnect on every scheduled refresh.

**Files:**
- Modify: `src/bl.cpp` (line ~1910, the `WiFi.disconnect(true)` inside `downloadAndShow()`)

**Step 1: Add X4 awake-loop guard around WiFi disconnect**

In `downloadAndShow()`, find the `WiFi.disconnect(true)` call after image download (line ~1910):

```cpp
WiFi.disconnect(true); // no need for WiFi, save power starting here
```

Replace with:

```cpp
#ifdef BOARD_XTEINK_X4
if (!x4_is_in_awake_loop())
#endif
{
  WiFi.disconnect(true); // no need for WiFi, save power starting here
}
```

**Step 2: Also guard the second WiFi.disconnect in the cached-image path**

Find the other `WiFi.disconnect` inside `downloadAndShow()` (the cached-image path, line ~1266 area, search for `WiFi.disconnect(true)` in bl.cpp) and apply the same guard.

**Step 3: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 4: Commit**

```bash
git add src/bl.cpp
git commit -m "perf(x4): skip WiFi disconnect during awake loop refresh"
```

---

### Task 2: Skip mDNS restart on awake-loop reconnect

**Objective:** Avoid calling `MDNS.begin()` on every WiFi reconnect inside the awake loop. mDNS needs to be started once, not on every reconnect.

**Files:**
- Modify: `src/wifi_network.cpp:67` (the `MDNS.begin()` call)

**Step 1: Add a static flag to prevent mDNS re-init**

In `src/wifi_network.cpp`, find the mDNS start line (line ~67):

```cpp
Log_info("mDNS started: %s", ...);
MDNS.begin(...)
```

Add a static flag:

```cpp
static bool mdns_started = false;
// ...
if (!mdns_started) {
  Log_info("mDNS started: %s", ...);
  MDNS.begin(...);
  mdns_started = true;
}
```

**Step 2: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 3: Commit**

```bash
git add src/wifi_network.cpp
git commit -m "perf(x4): skip mDNS restart on WiFi reconnect"
```

---

### Task 3: Early-exit button poll when no button pressed

**Objective:** Reduce the 2.5s blocking poll in `x4_poll_buttons_after_wakeup()` to ~200ms when no button is being pressed.

**Files:**
- Modify: `src/x4_buttons.cpp` (function `x4_poll_buttons_after_wakeup`)

**Step 1: Add early-exit after initial sampling window**

Replace `x4_poll_buttons_after_wakeup()` with:

```cpp
X4Button x4_poll_buttons_after_wakeup(void)
{
  X4Button last_button = X4_BTN_NONE;
  int debounce_count = 0;
  unsigned long poll_start = millis();

  // Quick check: if no button pressed in first 200ms, bail out early
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

    // Early exit: if past the quick-check window and no button seen at all
    if (millis() > quick_check_end && last_button == X4_BTN_NONE && debounce_count == 0)
    {
      return X4_BTN_NONE;
    }

    delay(X4_BUTTON_POLL_INTERVAL_MS);
  }

  return X4_BTN_NONE;
}
```

**Step 2: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 3: Commit**

```bash
git add src/x4_buttons.cpp
git commit -m "perf(x4): early-exit button poll when no button pressed (2.5s -> 200ms)"
```

---

### Task 4: Remove duplicate submitStoredLogs call

**Objective:** `submitStoredLogs()` is called twice in `bl_init()` — once before `downloadAndShow()` (line ~1337) and once after (line ~1390). Consolidate to a single call after `downloadAndShow()`.

**Files:**
- Modify: `src/bl.cpp` (lines ~1337 and ~1390)

**Step 1: Remove the first submitStoredLogs call**

Find the first `submitStoredLogs()` call (line ~1337, before `downloadAndShow()`) and remove it or comment it out. Keep only the one after `downloadAndShow()`.

**Step 2: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 3: Commit**

```bash
git add src/bl.cpp
git commit -m "perf(x4): remove duplicate submitStoredLogs call"
```

---

### Task 5: Skip list_files() on X4 boot

**Objective:** `list_files()` iterates all SPIFFS files and logs each one on every boot. This adds I/O and log overhead with no functional benefit.

**Files:**
- Modify: `src/bl.cpp` (find the `list_files()` call)

**Step 1: Guard list_files() for X4**

Find the `list_files()` call in `bl_init()` and wrap it:

```cpp
#ifndef BOARD_XTEINK_X4
  list_files();
#endif
```

**Step 2: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 3: Commit**

```bash
git add src/bl.cpp
git commit -m "perf(x4): skip list_files() on boot"
```

---

### Task 6: Reduce log verbosity on hot paths

**Objective:** Remove logging from `filesystem_file_exists()` and `display_read_file()` which are called frequently during navigation and image display.

**Files:**
- Modify: `src/filesystem.cpp` (the `filesystem_file_exists()` function)
- Modify: `src/display.cpp` (the `display_read_file()` function — search for `Serial.println` / `Serial.printf`)

**Step 1: Remove log from filesystem_file_exists()**

In `src/filesystem.cpp`, find `filesystem_file_exists()` and remove the `Log_info("file ... exists/doesn't exist")` lines:

```cpp
bool filesystem_file_exists(const char *name) {
  // Removed Log_info — hot path, called on every playlist navigation
  return SPIFFS.exists(name) || SPIFFS.exists(String("/") + name);
}
```

**Step 2: Replace Serial.println in display_read_file with Log_info**

In `src/display.cpp`, find `display_read_file()` and replace direct `Serial.println`/`Serial.printf` calls with `Log_info` (which is gated by DEV_FIRMWARE via ArduinoLog):

```cpp
// Replace: Serial.printf("Reading %s...\n", name);
// With: Log_info("Reading %s", name);
```

**Step 3: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 4: Commit**

```bash
git add src/filesystem.cpp src/display.cpp
git commit -m "perf(x4): reduce log verbosity on hot paths"
```

---

### Task 7: Skip heap integrity check in awake loop

**Objective:** `heap_caps_check_integrity_all(true)` is called during image download (line ~1846). This is a debug check that walks all heap metadata. Skip it in the awake loop.

**Files:**
- Modify: `src/bl.cpp` (line ~1846, the `heap_caps_check_integrity_all` call)

**Step 1: Guard the heap check**

Find `heap_caps_check_integrity_all` in bl.cpp and wrap:

```cpp
#ifdef BOARD_XTEINK_X4
if (!x4_is_in_awake_loop())
#endif
{
  heap_caps_check_integrity_all(true);
}
```

**Step 2: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 3: Commit**

```bash
git add src/bl.cpp
git commit -m "perf(x4): skip heap integrity check in awake loop"
```

---

### Task 8: Batch image download with buffer reads

**Objective:** Image download currently uses byte-by-byte `stream->read()` (line ~1868). Replace with `readBytes()` using a buffer for faster downloads.

**Files:**
- Modify: `src/bl.cpp` (the image download loop in `downloadAndShow()`, line ~1854-1879)

**Step 1: Replace byte-by-byte read with buffered read**

Find the download loop:
```cpp
Downloading image with WifiClient (stream)
...
int bytesRead = stream->read();  // byte-by-byte
```

Replace with a buffered approach:
```cpp
uint8_t readBuf[1024];
while (counter < content_size) {
  int avail = stream->available();
  if (avail > 0) {
    int toRead = min((size_t)avail, sizeof(readBuf));
    int bytesRead = stream->readBytes(readBuf, toRead);
    if (bytesRead <= 0) break;
    memcpy(buffer + counter, readBuf, bytesRead);
    counter += bytesRead;
  } else {
    delay(1);
  }
}
```

**Step 2: Build and verify**

Run: `pio run -e xteink_x4`
Expected: SUCCESS

**Step 3: Commit**

```bash
git add src/bl.cpp
git commit -m "perf(x4): batch image download with 1KB buffer reads"
```

---

## Verification

After all tasks:

```bash
# Build X4
pio run -e xteink_x4

# Build trmnl (ensure no breakage)
pio run -e trmnl

# Expected: both SUCCESS
```

### Expected improvements

| Optimization | Before | After | Savings |
|---|---|---|---|
| Early-exit button poll | 2500ms | ~200ms | **~2.3s per button-wake cycle** |
| Skip WiFi disconnect in awake loop | Full reconnect (~1-3s) per refresh | No reconnect | **~1-3s per awake-loop refresh** |
| Skip mDNS restart | ~50-100ms per reconnect | 0ms | **~50-100ms per reconnect** |
| Remove duplicate submitStoredLogs | 1 extra HTTP POST (~500ms) | 0 | **~500ms per cycle** |
| Skip list_files() | O(n) file iteration + logs | 0 | **~50-500ms per boot** |
| Reduce hot-path logging | Multiple log lines per nav | 0 | **~10-50ms per nav press** |
| Skip heap integrity check | ~10-50ms per download | 0 in awake loop | **~10-50ms per awake refresh** |
| Batch image download | byte-by-byte (~1ms per KB) | 1KB buffer (~0.1ms per KB) | **~90ms per 90KB image** |

### Total estimated savings
- **Timer-wake cycle**: ~0.5-1s faster
- **Button-wake cycle**: ~2.8-3.3s faster (dominated by early-exit poll)
- **Awake-loop refresh**: ~1.2-3.5s faster per refresh cycle

---

## Risks & Tradeoffs

1. **Skipping WiFi disconnect in awake loop**: WiFi stays connected between refreshes, using more power. But device is USB-powered so power isn't a concern. Tradeoff: acceptable.

2. **Early-exit button poll**: If user presses button >200ms after power button release, it won't be detected. Tradeoff: acceptable — the awake loop handles continuous button polling once USB is connected.

3. **Removing list_files()**: Loses debug visibility of SPIFFS contents. Tradeoff: acceptable — can re-enable with a debug flag if needed.

4. **Batch image download**: Changes download loop logic. Risk: must handle partial reads and timeouts correctly. Tradeoff: test carefully with different image sizes.

5. **All changes are X4-only**: No risk to other board targets.