//
// Created by guizo on 20/11/2025.
//

#ifndef REQUEST_H
#define REQUEST_H

#include "http_types.h"
#include "common.h"

http_request_line *parseRequestLine(const char *buff, const size_t bufSize, char *delimiter);
void freeRequestLine(http_request_line* req);


#endif //REQUEST_H
