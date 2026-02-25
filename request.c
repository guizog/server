//
// Created by guizo on 20/11/2025.
//

#include "request.h"

http_request_line *parseRequestLine(const char *buff, const size_t bufSize, char *delimiter) {
    char *copyBuffer = malloc(bufSize);
    if (!copyBuffer) return NULL;

    http_request_line *req = malloc(sizeof(http_request_line));
    if (!req) {
        free(copyBuffer);
        return NULL;
    }

    char *saveptr1;
    char *saveptr2;

    memcpy(copyBuffer, buff, bufSize);
    char *reqToken = strtok_r(copyBuffer, CRLF, &saveptr1);

    req->method = strdup(strtok_r(reqToken, SP, &saveptr2));
    req->pathUri = strdup(strtok_r(NULL, SP, &saveptr2));
    //printf("===>Method: %s\n", req->method);
    //printf("===>pathUri: %s\n", req->pathUri);

    for (int i = 0; reqToken != NULL; i++) {
        //printf("=>token: %s\n", reqToken);
        reqToken = strtok_r(NULL, CRLF, &saveptr1);
    }

    free(copyBuffer);
    return req;
}

void freeRequestLine(http_request_line* req) {
    free(req->pathUri);
    free(req->method);
    free(req);
}