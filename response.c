//
// Created by guizo on 20/11/2025.
//

#include "response.h"

int setStatusCode(http_response_line* resp, int statusCode) {
    for (int i = 0; i < STATUS_CODES_COUNT; i++) {
        http_status_code curr = status_codes[i];
        if (curr.code == statusCode) {
            resp->statusCode = curr;
            return 0;
        }
    }

    return 1;
}

int buildResponsePayload(http_response_line* resp, char *body, const size_t bodyLength, const char *contentType, const http_header *headers, const size_t headersCount) {
    if (!resp || !resp->statusCode.text)
        return 1;

    if (!contentType)
        contentType = "application/octet-stream";

    int totalSize = 0;
    totalSize += snprintf(NULL, 0, "%s %d %s\r\n", resp->version, resp->statusCode.code, resp->statusCode.text);
    totalSize += snprintf(NULL, 0, "Content-Type: %s\r\n", contentType);
    totalSize += snprintf(NULL, 0, "Content-Length: %zu\r\n", bodyLength);

    for (size_t i = 0; i < headersCount; i++) {
        if (!headers[i].name || !headers[i].value)
            continue;
        totalSize += snprintf(NULL, 0, "%s: %s\r\n", headers[i].name, headers[i].value);
    }
    totalSize += 2; //  \r\n
    totalSize += bodyLength;

    //3 spaces + "\r\n\r\n" (4 chars) + null terminator
    resp->payloadBody = malloc(totalSize + 1);
    if (!resp->payloadBody) {
        perror("malloc failed");
        return 1;
    }
    int offset = 0;

    offset += snprintf(resp->payloadBody + offset,
                       totalSize - offset + 1,
                       "%s %d %s\r\n",
                       resp->version,
                       resp->statusCode.code,
                       resp->statusCode.text);

    offset += snprintf(resp->payloadBody + offset,
                       totalSize - offset + 1,
                       "Content-Length: %zu\r\n",
                       bodyLength);

    offset += snprintf(resp->payloadBody + offset,
                       totalSize - offset + 1,
                       "Content-Type: %s\r\n",
                       contentType);

    for (size_t i = 0; i < headersCount; i++) {
        if (!headers[i].name || !headers[i].value)
            continue;

        offset += snprintf(resp->payloadBody + offset,
                           totalSize - offset + 1,
                           "%s: %s\r\n",
                           headers[i].name,
                           headers[i].value);
    }

    offset += snprintf(resp->payloadBody + offset,
                       totalSize - offset + 1,
                       "\r\n");

    if (body && bodyLength > 0) {
        memcpy(resp->payloadBody + offset, body, bodyLength);
        offset += bodyLength;
    }

    resp->payloadBody[offset] = '\0';
    resp->payloadLength = offset;
    return 0;
}