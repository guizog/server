//
// Created by guizo on 03/04/2026.
//

#include "job_queue.h"
#include "common.h"

job_queue_t queue;

void queue_init(job_queue_t *q) {
    q->head = q->tail = NULL;
    q->size = 0;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void enqueue(job_queue_t *q, int client_fd) {
    job_t *job = malloc(sizeof(*job));
    if (!job) return;

    job->client_fd = client_fd;
    job->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (q->size >= QUEUE_LIMIT) {
        pthread_mutex_unlock(&q->mutex);
        closesocket((SOCKET) client_fd);
        free(job);
        return;
    }

    if (!q->tail) {
        q->head = q->tail = job;
    }
    else {
        q->tail->next = job;
        q->tail = job;
    }

    q->size++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}