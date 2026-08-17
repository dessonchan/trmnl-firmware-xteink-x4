# TRMNL Server: Action Button API Implementation

## Overview

The firmware now sends action button presses to a new API endpoint. The server needs to implement this endpoint to handle button actions and return display data.

## New API Endpoint: `GET /api/action/:button`

### Request

**URL:** `GET /api/action/:button` where `:button` is one of `back`, `confirm`, `left`, `right`

**Headers:** Identical to `/api/display`, plus one new header:

| Header | Example | Description |
|--------|---------|-------------|
| `X-Buttons` | `back,right,left,confirm` | **NEW** — Comma-separated list of action buttons available on this device |

All existing `/api/display` headers are also sent (ID, Access-Token, FW-Version, Refresh-Rate, Battery-Voltage, RSSI, etc.).

### Response: Success with Image

Same JSON format as `/api/display`:

```json
{
  "status": 0,
  "image_url": "https://server.com/images/next_screen.bmp",
  "filename": "weather_2026-08-17T12:00:00Z",
  "refresh_rate": 600
}
```

### Response: No Update

```
HTTP/1.1 204 No Content
```

The firmware will keep the current screen and continue normal operation.

### Response: Error

The firmware will keep the current screen and continue normal operation.

## Modified Existing Endpoint: `GET /api/display`

One new header is added when the device has action buttons:

```
X-Buttons: back,right,left,confirm
```

**Backward compatible:** If `X-Buttons` header is absent, the device has no action buttons (original behavior unchanged).

## Suggested Server Logic

```
GET /api/action/:button
  1. Authenticate device via ID + Access-Token headers
  2. Read X-Buttons header to know device capabilities
  3. Determine what the :button action should do based on device state
  4. If there's a new image to show:
     → Return full /api/display JSON with image_url
  5. If no update needed:
     → Return HTTP 204 No Content
```

## Example Flows

### Flow 1: User presses "Right" button

```
Firmware → GET /api/action/right
           Headers: ID: A4:CF:12:34:56:78
                    Access-Token: abc123
                    X-Buttons: back,right,left,confirm
                    ... (other display headers)

Server   → 200 OK
           Body: {"status":0, "image_url":"https://.../next.bmp", "filename":"...", "refresh_rate":600}
```

### Flow 2: User presses "Back" but no previous screen

```
Firmware → GET /api/action/back
           Headers: ID: A4:CF:12:34:56:78
                    Access-Token: abc123
                    X-Buttons: back,right,left,confirm

Server   → 204 No Content
```