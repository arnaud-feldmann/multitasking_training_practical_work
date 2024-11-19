#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include "acquisitionManager.h"
#include "msg.h"
#include "iSensor.h"
#include "multitaskingAccumulator.h"
#include "iAcquisitionManager.h"
#include "debug.h"
#define NB_CASES 1000

//producer count storage
volatile unsigned int producedCount = 0;

pthread_t producers[4];

static void *produce(void *params);

/**
* Semaphores and Mutex
*/

pthread_mutex_t mutex_i_plein, mutex_i_libre, mutex_produced_count;
sem_t libres,pleines;
int i_indice_libre[NB_CASES], i_indice_plein[NB_CASES];
MSG_BLOCK messageBuffer[NB_CASES];

/*
* Creates the synchronization elements.
* @return ERROR_SUCCESS if the init is ok, ERROR_INIT otherwise
*/
static unsigned int createSynchronizationObjects(void);

/*
* Increments the produce count.
*/
static void incrementProducedCount(void);

static unsigned int createSynchronizationObjects(void)
{
    unsigned int ret = 0;
    for (int i = 0 ; i < NB_CASES ; i++) i_indice_libre[i] = i;
    ret |= sem_init(&libres, 0, NB_CASES);
    ret |= sem_init(&pleines, 0, 0);
    ret |= pthread_mutex_init(&mutex_i_plein, NULL);
    ret |= pthread_mutex_init(&mutex_i_libre, NULL);
    ret |= pthread_mutex_init(&mutex_produced_count, NULL);
    if (ret) return ERROR_INIT;
	printf("[acquisitionManager] Semaphore created\n");
	return ERROR_SUCCESS;
}

static void incrementProducedCount(void)
{
    pthread_mutex_lock(&mutex_produced_count);
    producedCount++;
    pthread_mutex_unlock(&mutex_produced_count);
}

unsigned int getProducedCount(void)
{
	unsigned int p = 0;
    pthread_mutex_lock(&mutex_produced_count);
    p = producedCount;
    pthread_mutex_unlock(&mutex_produced_count);
	return p;
}

MSG_BLOCK getMessage(void){
    MSG_BLOCK res;
    static int i_libre = 0, i_plein = 0;
    int i;
    sem_wait(&pleines);
    i = i_indice_plein[i_plein];
    i_plein = (i_plein + 1) % NB_CASES; /* pas de mutex car monoread */
    res = messageBuffer[i];
    i_indice_libre[i_libre] = i;
    i_libre = (i_libre + 1) % NB_CASES;
    sem_post(&libres);
    return res;
}

//TODO create accessors to limit semaphore and mutex usage outside of this C module.

unsigned int acquisitionManagerInit(void)
{
	unsigned int i;
	printf("[acquisitionManager] Synchronization initialization in progress...\n");
	fflush( stdout );
	if (createSynchronizationObjects() == ERROR_INIT)
		return ERROR_INIT;
	
	printf("[acquisitionManager] Synchronization initialization done.\n");

	for (i = 0; i < PRODUCER_COUNT; i++)
	{
        pthread_create(&producers[i], NULL, produce, (void *) (long)i);
	}

	return ERROR_SUCCESS;
}

void acquisitionManagerJoin(void)
{
	unsigned int i;
	for (i = 0; i < PRODUCER_COUNT; i++)
	{
        pthread_join(producers[i], NULL);
	}

    sem_destroy(&pleines);
    sem_destroy(&libres);
    pthread_mutex_destroy(&mutex_i_plein);
    pthread_mutex_destroy(&mutex_i_libre);
    pthread_mutex_destroy(&mutex_produced_count);
	printf("[acquisitionManager] Semaphore cleaned\n");
}

void multiWrite(MSG_BLOCK message) {
    int i;
    static int i_libre = 0, i_plein = 0;
    sem_wait(&libres);
    pthread_mutex_lock(&mutex_i_libre);
    i = i_indice_libre[i_libre];
    i_libre = (i_libre + 1) % NB_CASES;
    pthread_mutex_unlock(&mutex_i_libre);
    messageBuffer[i] = message;
    pthread_mutex_lock(&mutex_i_plein);
    i_indice_plein[i_plein] = i;
    i_plein = (i_plein + 1) % NB_CASES;
    pthread_mutex_unlock(&mutex_i_plein);
    incrementProducedCount();
    sem_post(&pleines);
}

void *produce(void* params)
{
	D(printf("[acquisitionManager] Producer created with id %ld\n", pthread_self()));
	unsigned int i = 0;
    MSG_BLOCK message;
	while (i < PRODUCER_LOOP_LIMIT)
	{
		i++;
		sleep(PRODUCER_SLEEP_TIME+(rand() % 5));
        getInput((int) (long)params, &message);
        multiWrite(message);
	}
	printf("[acquisitionManager] %ld termination\n", pthread_self());
    acquisitionManagerJoin();
    return NULL;
}
