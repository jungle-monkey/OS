#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <stdio.h>
#include <pthread.h>

#include "multi_mutex.h"

static pthread_mutex_t LOCK = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t LOCK2 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

// arr of pointers, if it is NULL -> end of arr
int multi_mutex_unlock(pthread_mutex_t **mutexv) 
{
    // printf("unlock!\n");

    pthread_mutex_lock(&LOCK2);
    while (*mutexv)
    {
        // printf("unlock!\n");
        // built-in mutex returns error if unsuccessful, 0 if successful
        int return_value = pthread_mutex_unlock(*mutexv); 

        if (return_value != 0)
        {
            pthread_mutex_unlock(&LOCK2);
            return -1;
        }
        mutexv++;
    }
    pthread_mutex_unlock(&LOCK2);
    return 0;
}

int multi_mutex_trylock(pthread_mutex_t **mutexv)
{
    // printf("trylock!\n");
    pthread_mutex_lock(&LOCK);
    // Store the beginning of the arr
    pthread_mutex_t **beginning_arr = mutexv;
    while (*mutexv)
    {
        // printf("trylock!\n");
        int was_successful = pthread_mutex_trylock(*mutexv);

        if (was_successful != 0)
        {
            while (*beginning_arr != *mutexv)
            {
                pthread_mutex_unlock(*beginning_arr);
                beginning_arr++;
            }
            pthread_mutex_unlock(&LOCK);
            return -1;
        }
        mutexv++;
    }
    pthread_mutex_unlock(&LOCK);
    return 0;
}
