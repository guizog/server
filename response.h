//
// Created by guizo on 20/11/2025.
//

#ifndef RESPONSE_H
#define RESPONSE_H

#include "common.h"
#include "http_types.h"

int setStatusCode(http_response_line* resp, int statusCode);
int buildResponsePayload(http_response_line* resp, char *body, const size_t bodyLength, const char *contentType, const http_header *headers, const size_t headersCount);

#endif //RESPONSE_H
