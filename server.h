//
// Created by guizo on 16/11/2025.
//

#ifndef SERVER_H
#define SERVER_H

#include "common.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define CRLF "\r\n"
#define SP " "
#define _WIN32_WINNT 0x501

struct http_status_code {
    int code;
    char *text;
}typedef http_status_code;

struct http_request_line {
    char *method;
    char *pathUri;
    char *payload;
    char *version;
}typedef http_request_line;

struct http_response_line {
    http_status_code statusCode;
    char *payload;
    char *version;
}typedef http_response_line;

int startServer();
int handleClient(int);

#endif //SERVER_H
