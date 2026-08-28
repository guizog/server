//
// Created by guizo on 20/11/2025.
//

#ifndef HTTP_TYPES_H
#define HTTP_TYPES_H

#define CRLF "\r\n"
#define SP " "
#include <stddef.h>

struct http_status_code {
    int code;
    char *text;
}typedef http_status_code;

struct http_header {
    char *name;
    char *value;
}typedef http_header;

extern const http_status_code status_codes[];

extern const size_t STATUS_CODES_COUNT;


#endif //HTTP_TYPES_H
