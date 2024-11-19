#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "displayManager.h"
#include "iDisplay.h"
#include "iMessageAdder.h"
#include "msg.h"
#include "multitaskingAccumulator.h"
#include "debug.h"

// DisplayManager thread.
pthread_t displayThread;

/**
 * Display manager entry point.
 * */
static void *display( void *parameters );


void displayManagerInit(void){
    pthread_create(&displayThread, NULL, display, NULL);
}

void displayManagerJoin(void){
    pthread_join(displayThread, NULL);
} 

static void *display( void *parameters )
{
    D(printf("[displayManager] Thread created for display with id %ld\n", pthread_self()));
    unsigned int diffCount = 0;
    stats_t stats;

    while(diffCount < DISPLAY_LOOP_LIMIT){
        sleep(DISPLAY_SLEEP_TIME);
        stats = getStats();
        print(stats.producedCount, stats.consumedCount);
        messageDisplay(&stats.currentSum);
    }

    printf("[displayManager] %ld termination\n", pthread_self());
    return NULL;
}
