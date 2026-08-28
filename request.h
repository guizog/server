//
// Created by guizo on 20/11/2025.
//

#ifndef REQUEST_H
#define REQUEST_H

#include "http_types.h"
#include "common.h"

struct http_request_line {
    char *method;
    char *pathUri;
    char *version;

    http_header *headers;
    size_t headersCount;

    char *payloadBody;
    size_t payloadLength;
}typedef http_request_line;

http_request_line *parseRequestLine(const char *buff, const size_t bufSize, char *delimiter);
void freeRequestLine(http_request_line* req);


#endif //REQUEST_H
