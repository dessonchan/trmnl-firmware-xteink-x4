#include <api-client/action.h>
#include <api-client/request_headers.h>
#include <api_response_parsing.h>
#include <config.h>
#include <globals.h>
#include <http_client.h>
#include <HTTPClient.h>
#include <inttypes.h>
#include <misc/sensor.h>
#include <trmnl_log.h>

#ifdef BOARD_XTEINK_X4
#include <buttons_config.h>
#endif

ApiActionResult fetchApiAction(ApiDisplayInputs &apiDisplayInputs, const char *actionName)
{
  String url = apiDisplayInputs.baseUrl + "/api/action/" + String(actionName);

  return withHttp(
    url,
    [&apiDisplayInputs, actionName](HTTPClient *https, HttpError error) -> ApiActionResult {
      if (error == HttpError::HTTPCLIENT_WIFICLIENT_ERROR) {
        Log_error("[ACTION] Unable to create WiFiClient");
        return ApiActionResult{
            .error = https_request_err_e::HTTPS_UNABLE_TO_CONNECT,
            .no_content = false,
            .response = {},
            .error_detail = "Unable to create WiFiClient",
        };
      }
      if (error == HttpError::HTTPCLIENT_HTTPCLIENT_ERROR) {
        Log_error("[ACTION] Unable to create HTTPClient");
        return ApiActionResult{
            .error = https_request_err_e::HTTPS_UNABLE_TO_CONNECT,
            .no_content = false,
            .response = {},
            .error_detail = "Unable to create HTTPClient",
        };
      }

      // Use shorter timeout for action requests (5s) — buttons should feel responsive
      https->setTimeout(5000);
      https->setConnectTimeout(5000);

      // Build headers — same as /api/display plus X-Buttons (if available)
#ifdef BOARD_XTEINK_X4
      HttpHeaderList headers = buildDisplayHeaders(apiDisplayInputs, x4_action_buttons_list());
#else
      HttpHeaderList headers = buildDisplayHeaders(apiDisplayInputs);
#endif
      if (sensor().buildSensorsHeader(nullptr)) {
        // Sensors header is added inside addHeaders if available
      }

      char *szTemp = nullptr;
      if (sensor().buildSensorsHeader(&szTemp)) {
        headers.push_back({"SENSORS", szTemp});
        free(szTemp);
      }

      applyHeaders(*https, headers);
      logHeaders(headers);

      Log_info("[ACTION] GET /api/action/%s", actionName);
      int httpCode = https->GET();

      if (httpCode == HTTP_CODE_PERMANENT_REDIRECT || httpCode == HTTP_CODE_TEMPORARY_REDIRECT) {
        String location = https->getLocation();
        https->end();
        String redirectUrl = (location.startsWith("http://") || location.startsWith("https://"))
                               ? location
                               : (apiDisplayInputs.baseUrl + location);
        https->begin(redirectUrl);
        Log_info("[ACTION] Redirected to: %s", redirectUrl.c_str());
        https->setTimeout(5000);
        https->setConnectTimeout(5000);
        applyHeaders(*https, headers);
        httpCode = https->GET();
      }

      // 204 No Content — server says no update needed
      if (httpCode == HTTP_CODE_NO_CONTENT) {
        Log_info("[ACTION] 204 No Content — no update for action '%s'", actionName);
        return ApiActionResult{
            .error = https_request_err_e::HTTPS_NO_ERR,
            .no_content = true,
            .response = {},
            .error_detail = "",
        };
      }

      if (httpCode < 0 || httpCode != HTTP_CODE_OK) {
        Log_error("[ACTION] GET failed, error: %s (%d)", https->errorToString(httpCode).c_str(), httpCode);
        return ApiActionResult{
            .error = https_request_err_e::HTTPS_RESPONSE_CODE_INVALID,
            .no_content = false,
            .response = {},
            .error_detail = "HTTP error: " + https->errorToString(httpCode) + " (" + String(httpCode) + ")",
        };
      }

      Log_info("[ACTION] GET OK, code: %d", httpCode);
      String payload = https->getString();
      Log_info("[ACTION] Payload - %s", payload.c_str());

      auto apiResponse = parseResponse_apiDisplay(payload);
      if (apiResponse.outcome == ApiDisplayOutcome::DeserializationError) {
        return ApiActionResult{
            .error = https_request_err_e::HTTPS_JSON_PARSING_ERR,
            .no_content = false,
            .response = {},
            .error_detail = "JSON parse failed: " + apiResponse.error_detail,
        };
      }

      return ApiActionResult{
          .error = https_request_err_e::HTTPS_NO_ERR,
          .no_content = false,
          .response = apiResponse,
          .error_detail = "",
      };
    },
    /*resumable=*/true);
}