//
// Created by guizo on 20/11/2025.
//

#ifndef RESPONSE_H
#define RESPONSE_H

#include "common.h"
#include "http_types.h"

int setStatusCode(http_response_line* resp, int statusCode);
int buildResponsePayload(http_response_line* resp, char* fileBuffer);

#endif //RESPONSE_H
