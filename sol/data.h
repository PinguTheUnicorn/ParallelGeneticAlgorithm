#ifndef DATA_H
#define DATA_H

#include "sack_object.h"
#include "individual.h"
#include <pthread.h>

// structure to pass on variables for thread function(run_genetic_algorithm)
typedef struct data
{
    int thread_id;
    sack_object **objects;
    int object_count;
    int sack_capacity;
    int generations_count;
    int P;
    individual **current_generation;
    individual **next_generation;
    pthread_barrier_t *bariera;
} data;

#endif