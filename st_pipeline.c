#include "st_pipeline.h"
#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

activeObject *first_AO, *second_AO, *third_AO, *fourth_AO;

bool is_prime(unsigned int num)
{
    if (num < 2)
        return false;
    if (num == 2)
        return true;
    if (num % 2 == 0)
        return false;
    for (unsigned int i = 3; i <= sqrt(num); i += 2)
    {
        if (num % i == 0)
            return false;
    }
    return true;
}

void enqueue(queue *q, void *item)
{
    node *new_node = malloc(sizeof(node));
    if (new_node == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for new node\n");
        exit(EXIT_FAILURE);
    }

    new_node->data = item;
    new_node->next = NULL;

    pthread_mutex_lock(&(q->mutex));
    if (q->tail != NULL)
    {
        q->tail->next = new_node;
    }
    else
    {
        q->head = new_node;
    }
    q->tail = new_node;
    pthread_cond_signal(&(q->cond));
    pthread_mutex_unlock(&(q->mutex));
}

void *dequeue(queue *q)
{
    pthread_mutex_lock(&(q->mutex));
    while (q->head == NULL)
    {
        pthread_cond_wait(&(q->cond), &(q->mutex));
    }
    node *head = q->head;
    void *item = head->data;
    q->head = head->next;
    if (q->head == NULL)
    {
        q->tail = NULL;
    }
    pthread_mutex_unlock(&(q->mutex));

    free(head);
    return item;
}

void *run(void *arg)
{
    activeObject *self = (activeObject *)arg;
    void *task;
    while ((task = dequeue(self->q)) != NULL)
    {
        self->func(task);
    }
    return NULL;
}

activeObject *createActiveObject(void *(*func)(void *))
{
    activeObject *obj = malloc(sizeof(activeObject));
    if (obj == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for obj\n");
        exit(EXIT_FAILURE);
    }
    obj->q = malloc(sizeof(queue));
    if (obj->q == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for q\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(&(obj->q->mutex), NULL);
    pthread_cond_init(&(obj->q->cond), NULL);
    obj->q->head = NULL;
    obj->q->tail = NULL;
    obj->func = func;
    pthread_create(&(obj->worker), NULL, run, obj);
    return obj;
}

void freeQueue(queue *q)
{
    pthread_mutex_lock(&(q->mutex));
    node *current = q->head;
    while (current != NULL)
    {
        node *to_free = current;
        current = current->next;
        free(to_free);
    }
    pthread_mutex_unlock(&(q->mutex));
    pthread_mutex_destroy(&(q->mutex));
    pthread_cond_destroy(&(q->cond));
    free(q);
}

void stop(activeObject *obj)
{
    enqueue(obj->q, NULL);
    pthread_join(obj->worker, NULL);
    freeQueue(obj->q);
    free(obj);
}

queue *getQueue(activeObject *obj)
{
    return obj->q;
}

void *first_func(void *arg)
{
    unsigned int *number_ptr = (unsigned int *)arg;

    printf("%u\n%s\n", *number_ptr ,is_prime(*number_ptr) ? "true" : "false");

    // enqueue it to the next active object
    enqueue(getQueue(second_AO), number_ptr);

    return NULL;
}

void *second_func(void *arg)
{
    unsigned int *number = (unsigned int *)arg;

    // add 11 to the number
    *number += 11;

    // check if number is prime
    printf("%u\n%s\n", *number, is_prime(*number) ? "true" : "false");

    // enqueue it to the next active object
    enqueue(getQueue(third_AO), number);

    return NULL;
}

void *third_func(void *arg)
{
    unsigned int *number = (unsigned int *)arg;

    // subtract 13 from the number
    *number -= 13;

    // check if number is prime
    printf("%u\n%s\n", *number, is_prime(*number) ? "true" : "false");

    // enqueue it to the next active object
    enqueue(getQueue(fourth_AO), number);

    return NULL;
}



void *fourth_func(void *arg)
{
    unsigned int *number = (unsigned int *)arg;

    // add 2 to the number
    *number += 2;

    printf("%u\n", *number);

    // Cleanup
    free(number);

    return NULL;
}

int main(int argc, char **argv)
{
    // Check if argument is provided
    if (argc < 2)
    {
        printf("Please provide a number of iterations as argument.\n");
        return 1;
    }

    // Parse argument
    int iterations = atoi(argv[1]);
    int seed = argc == 3 ? atoi(argv[2]) : time(NULL);

    // Initialize the random number generator
    srand(seed);

    // Initialize the active objects
    first_AO = createActiveObject(first_func);
    second_AO = createActiveObject(second_func);
    third_AO = createActiveObject(third_func);
    fourth_AO = createActiveObject(fourth_func);

    // Generate random numbers and feed them to the first active object
    for (int i = 0; i < iterations-1; i++)
    {
        unsigned int *num = malloc(sizeof(unsigned int));
        *num = rand() % 900000 + 100000; // generate a 6 digit random number
        enqueue(getQueue(first_AO), num);
        sleep(1); // wait 1 second
        printf("\n");
    }

    // Stop the active objects
    stop(first_AO);
    stop(second_AO);
    stop(third_AO);
    stop(fourth_AO);

    return 0;
}
