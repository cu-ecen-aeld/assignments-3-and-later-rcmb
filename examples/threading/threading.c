#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

//Functions to add debug or error prints
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    //Cast the thread parameter to a pointer to the thread_data struct
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    //Wait before attempting to obtain the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    DEBUG_LOG("Wait over...obtaining mutex");

    //Attempt to obtain the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
      thread_func_args->thread_complete_success = false;
      ERROR_LOG("Failed to obtain mutex");
      return NULL;
    }
    
    //Wait before releasing the mutex
    usleep(thread_func_args->wait_to_release_ms * 1000);
    DEBUG_LOG("Wait over...releasing mutex");
    
    //Release the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
      thread_func_args->thread_complete_success = false;
      ERROR_LOG("Failed to release mutex");
      return NULL;
    }

    thread_func_args->thread_complete_success = true;
    
    //Return the thread parameter
    return thread_param;

}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * Allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    
    // allocate memory for thread_data
    struct thread_data* thread_data_ptr = malloc(sizeof(struct thread_data));
    if (thread_data_ptr == NULL) {
       ERROR_LOG("Failed to allocate memory for thread_data");
       return false;
    }

    // load thread_data
    thread_data_ptr->mutex = mutex;
    thread_data_ptr->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data_ptr->wait_to_release_ms = wait_to_release_ms;
    
    // create thread and pass thread_data as argument
    if (pthread_create(thread, NULL, threadfunc, (void*)thread_data_ptr) != 0) {
       ERROR_LOG("Failed to create thread");
       free(thread_data_ptr);
       return false;
    }

    return true;

}

