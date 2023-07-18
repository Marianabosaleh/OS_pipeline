#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

typedef struct node {
    void *data;
    struct node *next;
} node;

typedef struct queue {
    node *head;
    node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} queue;

typedef struct activeObject {
    pthread_t worker;
    queue *q;
    void *(*func)(void *);
} activeObject;