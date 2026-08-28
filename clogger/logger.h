//
// Created by guizo on 08/07/2026.
//
#pragma once

#ifndef LOGGER_H
#define LOGGER_H
#include <stddef.h>

#define writeLog(level, format, ...) \
        writeLog_internal(level, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
}log_level;

typedef struct {
    log_level level;
    char message[1024];

    char file[128];
    char function[64];
    int line;
}log_message;

typedef struct {
    log_message message;
    struct log_node* next;
}log_node;

typedef struct {
    size_t size;
    log_node* head;
    log_node* tail;
}log_queue;

int startLogger(const char *filename);
int shutdownLogger();

int writeLog_internal(log_level level, const char *file, const char *function, int line, const char *format, ...);

#endif //LOGGER_H
