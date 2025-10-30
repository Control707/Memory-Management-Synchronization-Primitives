// ============================================
// producer.c - Producer process implementation
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

    // Close semaphores (don't unlink - other processes may be using)
    if (mutex != NULL && mutex != SEM_FAILED) sem_close(mutex);
    if (empty != NULL && empty != SEM_FAILED) sem_close(empty);
    if (full != NULL && full != SEM_FAILED) sem_close(full);
}

void signal_handler(int sig) {
    printf("\nProducer: Caught signal %d, cleaning up...\n", sig);
    cleanup();
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <producer_id> <num_items>\n", argv[0]);
        exit(1);
    }

    int producer_id = atoi(argv[1]);
    int num_items = atoi(argv[2]);

    // Validate input
    if (producer_id <= 0 || num_items <= 0) {
        fprintf(stderr, "Error: producer_id and num_items must be positive\n");
        exit(1);
    }

    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Seed random number generator
    srand(time(NULL) + producer_id);

    // Attach to shared memory (create if doesn't exist)
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        exit(1);
    }

    buffer = (shared_buffer_t*)shmat(shm_id, NULL, 0);
    if (buffer == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize buffer on first attach (simple approach)
    // Note: In production, you'd want a more robust initialization mechanism
    static int first_producer = 1;
    if (first_producer) {
        buffer->head = 0;
        buffer->tail = 0;
        buffer->count = 0;
        first_producer = 0;
    }

    // Open semaphores (create if don't exist)
    mutex = sem_open(SEM_MUTEX, O_CREAT, 0644, 1);
    empty = sem_open(SEM_EMPTY, O_CREAT, 0644, BUFFER_SIZE);
    full = sem_open(SEM_FULL, O_CREAT, 0644, 0);

    if (mutex == SEM_FAILED || empty == SEM_FAILED || full == SEM_FAILED) {
        perror("sem_open failed");
        cleanup();
        exit(1);
    }

    printf("Producer %d: Starting to produce %d items\n", producer_id, num_items);

    // Main production loop
    for (int i = 0; i < num_items; i++) {
        // Create item with unique value
        item_t item;
        item.value = producer_id * 1000 + i;
        item.producer_id = producer_id;

        // Wait for empty slot
        sem_wait(empty);

        // Enter critical section
        sem_wait(mutex);

        // Add item to buffer (circular buffer)
        buffer->buffer[buffer->head] = item;
        buffer->head = (buffer->head + 1) % BUFFER_SIZE;
        buffer->count++;

        printf("Producer %d: Produced value %d\n", producer_id, item.value);

        // Exit critical section
        sem_post(mutex);

        // Signal item available
        sem_post(full);
        
        // Simulate production time
        usleep(rand() % 100000);
    }
    
    printf("Producer %d: Finished producing %d items\n", producer_id, num_items);
    cleanup();
    return 0;
}