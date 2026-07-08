//
// Created by guizo on 03/04/2026.
//
#pragma once

#include <pthread.h>
#include <stdlib.h>

#define THREAD_COUNT 4
#define QUEUE_LIMIT 1024

#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

typedef struct job_t {
    int client_fd;
    struct job_t *next;
} job_t;

typedef struct job_queue_t {
    job_t *head;
    job_t *tail;
    size_t size;

    pthread_mutex_t  mutex;
    pthread_cond_t cond;
} job_queue_t;

void queue_init(job_queue_t *q);
void enqueue(job_queue_t *q, int client_fd);
int dequeue(job_queue_t *q);

extern job_queue_t queue;

#endif //JOB_QUEUE_H
