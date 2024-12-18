#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h> 
#include <unistd.h>
#include <pthread.h>
#include "messageAdder.h"
#include "msg.h"
#include "iMessageAdder.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"
#include "iDisplay.h"

//consumer thread
pthread_t consumer;
//Message computed
volatile MSG_BLOCK currentSum;
//Consumer count storage
volatile unsigned int consumedCount = 0;
pthread_mutex_t mutex_current_sum;

/**
 * Increments the consume count.
 */
static void incrementConsumedCount(void);

/**
 * Consumer entry point.
 */
static void *sum( void *parameters );


MSG_BLOCK getCurrentSum(){
    MSG_BLOCK res;
    pthread_mutex_lock(&mutex_current_sum);
    res = currentSum;
    pthread_mutex_unlock(&mutex_current_sum);
    return res;
}

unsigned int getConsumedCount(){
    unsigned int res;
    res = consumedCount;
    return res;
}

void messageAdderInit(void){
    currentSum.checksum = 0;
    for (size_t i = 0; i < DATA_SIZE; i++)
    {
        currentSum.mData[i] = 0;
    }
    pthread_mutex_init(&mutex_current_sum, NULL);
    pthread_create(&consumer, NULL, sum, NULL);
}

void messageAdderJoin(void){
    pthread_join(consumer, NULL);
}

static void incrementConsumedCount(void) {
    consumedCount++;
}

stats_t getStats(){
    volatile MSG_BLOCK message;
    pthread_mutex_lock(&mutex_current_sum);
    message = currentSum;
    stats_t stats = {
        .producedCount = getProducedCount(),
        .consumedCount = getConsumedCount(),
        .currentSum = message
    };
    pthread_mutex_unlock(&mutex_current_sum);
    return stats;
}

static void *sum( void *parameters )
{
    D(printf("[messageAdder] Thread created for sum with id %ld\n", pthread_self()));
    unsigned int i = 0;
    MSG_BLOCK message;
    while(i<ADDER_LOOP_LIMIT){
        i++;
        sleep(ADDER_SLEEP_TIME);
        message = getMessage();
        if (messageCheck(&message)) {
            pthread_mutex_lock(&mutex_current_sum);
            messageAdd(&currentSum, &message);
            incrementConsumedCount();
            pthread_mutex_unlock(&mutex_current_sum);
        }
    }
    D(printf("[messageAdder] %ld termination\n", pthread_self()));
    pthread_mutex_destroy(&mutex_current_sum);
    return NULL;
}


