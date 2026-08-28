//
// Created by guizo on 20/11/2025.
//

#ifndef RESPONSE_H
#define RESPONSE_H

#include "common.h"
#include "http_types.h"

struct http_response_line {
    http_status_code statusCode;
    char *version;

    http_header *headers;
    size_t headersCount;

    char *payloadBody;
    size_t payloadLength;
}typedef http_response_line;

int setStatusCode(http_response_line* resp, int statusCode);
int buildResponsePayload(http_response_line* resp, char *body, const size_t bodyLength, const char *contentType, const http_header *headers, const size_t headersCount);
void freeResponseLine(http_response_line *resp);


#endif //RESPONSE_H
