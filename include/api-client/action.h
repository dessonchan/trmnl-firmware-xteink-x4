#pragma once

#include <HTTPClient.h>
#include <api_types.h>
#include <types.h>

// Result of calling /api/action/<button>
struct ApiActionResult {
  https_request_err_e error;
  bool no_content;         // true if server returned 204 No Content
  ApiDisplayResponse response;  // same format as /api/display when content is returned
  String error_detail;
};

/**
 * @brief Fetch action response from /api/action/<button>
 * @param apiDisplayInputs Standard display inputs (same headers as /api/display)
 * @param actionName Button action name (e.g. "back", "right", "left", "confirm")
 * @return ApiActionResult with error, no_content flag, and parsed response
 */
ApiActionResult fetchApiAction(ApiDisplayInputs &apiDisplayInputs, const char *actionName);