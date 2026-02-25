//
// Created by guizo on 20/11/2025.
//

#include "response.h"

int setStatusCode(http_response_line* resp, int statusCode) {
    for (int i = 0; i < STATUS_CODES_COUNT/sizeof(http_status_code); i++) {
        http_status_code curr = status_codes[i];
        if (curr.code == statusCode) {
            resp->statusCode = curr;
            return 0;
        }
    }

    return 1;
}

int buildResponsePayload(http_response_line* resp, char* fileBuffer) {
    size_t fileLength = strlen(fileBuffer);

    int codeDigits = snprintf(NULL, 0, "%d", resp->statusCode.code);
    size_t headersLength = strlen(resp->version) + codeDigits + strlen(resp->statusCode.text);

    //3 spaces + "\r\n\r\n" (4 chars) + null terminator
    resp->payload = malloc(headersLength + fileLength + 3 + 4 + 1);
    if (!resp->payload) {
        perror("malloc failed");
        return 1;
    }
    sprintf(resp->payload, "%s %d %s\r\n\r\n%s",
            resp->version,
            resp->statusCode.code,
            resp->statusCode.text,
            fileBuffer);

    return 0;
}