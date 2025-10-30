// ============================================
// buffer.h - Shared definitions
// Author: Zakaria Sherif
// Section: 04
// CS 3502: Operating Systems - Assignment 3
// ============================================
#ifndef BUFFER_H
#define BUFFER_H

// Required includes for both producer and consumer
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Constants for shared memory and semaphores
#define BUFFER_SIZE 10
#define SHM_KEY 0x1234
#define SEM_MUTEX "/sem_mutex"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"

// Item structure - represents data in the buffer
typedef struct {
    int value;          // Data value
    int producer_id;    // Which producer created this
} item_t;

// Shared buffer structure - circular buffer in shared memory
typedef struct {
    item_t buffer[BUFFER_SIZE];  // Array of items
    int head;                     // Next write position
    int tail;                     // Next read position
    int count;                    // Current items in buffer
} shared_buffer_t;

#endif