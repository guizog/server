//
// Created by guizo on 08/07/2026.
//

#include "logger.h"

#include <direct.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static log_queue queue;
static FILE *logFile = NULL;
static int running = 0;

static pthread_mutex_t queueMutex;
static pthread_cond_t queueCond;
static pthread_t loggerThread;

static void *loggerWorker(void *arg);

int startLogger(const char *dir) {
    queue.size = 0;
    queue.head = NULL;
    queue.tail = NULL;

    char logsDir[1024];
    snprintf(logsDir, sizeof(logsDir), "%s\\logs", dir);
    _mkdir(logsDir);

    char logPath[1024];
    snprintf(logPath, sizeof(logPath), "%s\\logs\\log-server.log", dir);
    logFile = fopen(logPath, "a");
    if (logFile == NULL) {
        perror("Failed to open logfile");
        return -1;
    }

    if (pthread_mutex_init(&queueMutex, NULL) != 0) {
        fclose(logFile);
        return -1;
    }

    if (pthread_cond_init(&queueCond, NULL) != 0) {
        pthread_mutex_destroy(&queueMutex);
        fclose(logFile);
        return -1;
    }

    running = 1;

    if (pthread_create(&loggerThread, NULL, loggerWorker, NULL) != 0) {
        running = 0;
        pthread_cond_destroy(&queueCond);
        pthread_mutex_destroy(&queueMutex);
        fclose(logFile);
        return -1;
    }

    return 0;
}

int shutdownLogger() {
    pthread_mutex_lock(&queueMutex);
    running = 0;
    pthread_cond_broadcast(&queueCond);
    pthread_mutex_unlock(&queueMutex);

    if (pthread_join(loggerThread, NULL) != 0) {
        return -1;
    }

    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }

    pthread_mutex_destroy(&queueMutex);
    pthread_cond_destroy(&queueCond);

    return 0;
}

const char *levelToString(log_level level) {
    switch (level) {
        case LOG_INFO:    return "[INFO]   ";
        case LOG_WARNING: return "[WARNING]";
        case LOG_ERROR:   return "[ERROR]  ";
        case LOG_DEBUG:   return "[DEBUG]  ";
        default:          return "[INFO]   ";
    }
}

void currentTimestamp(char* buffer, size_t size) {
    const time_t now = time(NULL);
    struct tm timeInfo;
    localtime_s(&timeInfo, &now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &timeInfo);
}

static const char *baseFilename(const char *path) {
    const char *lastSlash = strrchr(path, '/');

#ifdef _WIN32
    const char *lastBackslash = strrchr(path, '\\');
    if (lastBackslash && (!lastSlash || lastBackslash > lastSlash))
        lastSlash = lastBackslash;
#endif

    return lastSlash ? lastSlash + 1 : path;
}

static void enqueue(log_message *msg) {
    log_node *node = malloc(sizeof(log_node));
    if (!node) { return; }

    node->next = NULL;
    node->message = *msg;

    pthread_mutex_lock(&queueMutex);

    if (queue.tail == NULL) {
        queue.head = node;
        queue.tail = node;
    }
    else {
        queue.tail->next = node;
        queue.tail = node;
    }

    queue.size++;
    pthread_cond_signal(&queueCond);
    pthread_mutex_unlock(&queueMutex);
}

int writeLog_internal(log_level level, const char *file, const char *function, int line, const char *format, ...) {
    log_message msg;

    msg.level = level;
    strncpy(msg.file, baseFilename(file), sizeof(msg.file) - 1);
    msg.file[sizeof(msg.file) - 1] = '\0';
    strncpy(msg.function, function, sizeof(msg.function) - 1);
    msg.function[sizeof(msg.function) - 1] = '\0';
    msg.line = line;

    va_list args;
    va_start(args, format);
    vsnprintf(msg.message, sizeof(msg.message), format, args);
    va_end(args);

    enqueue(&msg);

    return 0;
}

static void *loggerWorker(void *arg) {
    (void) arg;

    for (;;) {
        pthread_mutex_lock(&queueMutex);

        while (queue.size == 0 && running) {
            pthread_cond_wait(&queueCond, &queueMutex);
        }

        if (queue.size == 0 && !running) {
            pthread_mutex_unlock(&queueMutex);
            break;
        }

        log_node *node = queue.head;
        queue.head = node->next;
        if (!queue.head) {
            queue.tail = NULL;
        }
        queue.size--;

        log_message msg = node->message;
        free(node);
        pthread_mutex_unlock(&queueMutex);

        char timestamp[20];
        currentTimestamp(timestamp, sizeof(timestamp));
        fprintf(logFile, "%s %s [%s:%d %s] %s\n",
                timestamp,
                levelToString(msg.level),
                msg.file,
                msg.line,
                msg.function,
                msg.message);
        fflush(logFile);
    }

    return NULL;
}
