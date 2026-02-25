//
// Created by guizo on 20/11/2025.
//

#include "request.h"

char *copyToken(const char *start, size_t length) {
    char *str = malloc(length + 1);
    if (!str) {
        return NULL;
    }
    memcpy(str, start, length);
    str[length] = '\0';
    return str;
}

http_request_line *parseRequestLine(const char *buff, const size_t bufSize, char *delimiter) {
    const char *cursor = buff;
    const char *end = buff + bufSize;

    http_request_line *req = calloc(1, sizeof(http_request_line));
    if (!req) {
        return NULL;
    }

    //  Parsing first request line
    const char *requestLineEnd = strstr(cursor, CRLF);
    if (!requestLineEnd) {
        free(req);
        return NULL;
    }

    const char *p1 = memchr(cursor, ' ', requestLineEnd - cursor);
    if (!p1) {
        free(req);
        return NULL;
    }
    const char *p2 = memchr(p1 + 1,  ' ', requestLineEnd - ( p1 + 1));
    if (!p2) {
        free(req);
        return NULL;
    }

    req->method = copyToken(cursor, p1 - cursor);
    req->pathUri = copyToken(p1 + 1, p2 - (p1 + 1));
    req->version = copyToken((p2 + 1), requestLineEnd - ( p2 + 1));

    cursor = requestLineEnd + 2;

    //  Parse headers line
    size_t headerCapacity = 8;
    size_t contentLength = 0;
    req->headers = malloc(headerCapacity * sizeof(http_header));
    if (!req->headers) {
        free(req);
        return NULL;
    }

    while (cursor < end) {
        if (cursor[0] == '\r' && cursor[1] == '\n') {
            cursor += 2;
            break;
        }

        const char *headerEnd = strstr(cursor, CRLF);
        if (!headerEnd) {
            free(req);
            return NULL;
        }

        const char *headerDelimiter = memchr(cursor, ':', headerEnd - cursor);
        if (!headerDelimiter) {
            free(req);
            return NULL;
        }

        //  dynamic array time
        if (req->headersCount == headerCapacity) {
            headerCapacity *= 2;

            http_header *temp = realloc(req->headers, headerCapacity * sizeof(http_header));
            if (!temp) {
                free(req);
                return NULL;
            }
            req->headers = temp;
        }

        req->headers[req->headersCount].name = copyToken(cursor, headerDelimiter - cursor);
        const char *valueStart = headerDelimiter + 1;
        if (*valueStart == ' ')
            valueStart++;

        req->headers[req->headersCount].value = copyToken(valueStart, headerEnd - valueStart);
        if (strcasecmp(req->headers[req->headersCount].name, "Content-Length") == 0) {
            contentLength = strtoul(req->headers[req->headersCount].value, NULL, 10);
        }

        req->headersCount++;
        cursor = headerEnd + 2;

    }

    //  Parsing body
    if (contentLength > 0) {
        if ((size_t)(end - cursor) < contentLength) {
            free(req);
            return NULL;
        }

        req->payloadBody = copyToken(cursor, contentLength);
        req->payloadLength = contentLength;
    }

    return req;
}

void freeRequestLine(http_request_line* req) {
    if (!req) return;

    free(req->pathUri);
    free(req->method);
    free(req->version);
    free(req->payloadBody);

    if (req->headers) {
        for (int i = 0; i < req->headersCount; i++) {
            free(req->headers[i].name);
            free(req->headers[i].value);
        }
        free(req->headers);
    }
    free(req);
}