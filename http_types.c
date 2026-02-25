//
// Created by guizo on 20/11/2025.
//

#include "http_types.h"

const http_status_code status_codes[] = {
    {200, " OK"},
    {400, " Bad Request"},
    {404, " Not Found"}
};

const size_t STATUS_CODES_COUNT = sizeof(status_codes) / sizeof(status_codes[0]);