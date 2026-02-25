//
// Created by guizo on 16/11/2025.
//

#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "http_types.h"
#include "request.h"
#include "response.h"

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define _WIN32_WINNT 0x501

int startServer();
int handleClient(int);

#endif //SERVER_H
