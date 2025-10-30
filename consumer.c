// ============================================
// consumer.c - Consumer process implementation
// Author: Zakaria Sherif
// Section: 04
// CS 3502: Operating Systems - Assignment 3
// ============================================
#include "buffer.h"

// Global variables for cleanup
shared_buffer_t* buffer = NULL;
sem_t* mutex = NULL;
sem_t* empty = NULL;
sem_t* full = NULL;
int shm_id = -1;

void cleanup() {
    // Detach shared memory
    if (buffer != NULL) {
        shmdt(buffer);
    }

    // Close semaphores
    if (mutex != NULL && mutex != SEM_FAILED) sem_close(mutex);
    if (empty != NULL && empty != SEM_FAILED) sem_close(empty);
    if (full != NULL && full != SEM_FAILED) sem_close(full);
}

void signal_handler(int sig) {
    printf("\nConsumer: Caught signal %d, cleaning up...\n", sig);
    cleanup();
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <consumer_id> <num_items>\n", argv[0]);
        exit(1);
    }

    int consumer_id = atoi(argv[1]);
    int num_items = atoi(argv[2]);

    // Validate input
    if (consumer_id <= 0 || num_items <= 0) {
        fprintf(stderr, "Error: consumer_id and num_items must be positive\n");
        exit(1);
    }

    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Seed random number generator
    srand(time(NULL) + consumer_id * 100);

    // Attach to shared memory (don't create - producer should create it)
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), 0666);
    if (shm_id < 0) {
        perror("shmget failed - make sure producer has been started");
        exit(1);
    }

    buffer = (shared_buffer_t*)shmat(shm_id, NULL, 0);
    if (buffer == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Open semaphores (don't create - producer creates them)
    mutex = sem_open(SEM_MUTEX, 0);
    empty = sem_open(SEM_EMPTY, 0);
    full = sem_open(SEM_FULL, 0);

    if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
        perror("sem_open failed - make sure producer has been started");
        cleanup();
        exit(1);
    }

    printf("Consumer %d: Starting to consume %d items\n", consumer_id, num_items);

    // Main consumption loop
    for (int i = 0; i < num_items; i++) {
        // Wait for full slot (item available)
        sem_wait(full);

        // Enter critical section
        sem_wait(mutex);

        // Remove item from buffer (circular buffer)
        item_t item = buffer->buffer[buffer->tail];
        buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
        buffer->count--;

        printf("Consumer %d: Consumed value %d from Producer %d\n",
               consumer_id, item.value, item.producer_id);

        // Exit critical section
        sem_post(mutex);

        // Signal empty slot available
        sem_post(empty);
        
        // Simulate consumption time
        usleep(rand() % 100000);
    }
    
    printf("Consumer %d: Finished consuming %d items\n", consumer_id, num_items);
    cleanup();
    return 0;
}