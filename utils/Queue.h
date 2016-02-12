/*
 * Queue.h
 *
 *  Created on: Feb 9, 2016
 *      Author: Aanal
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <inttypes.h>
#include "../uart/config.h"

/**
 * Data structure to ohold the queue in place
 */
struct Queue{
	uint8_t head;
	uint8_t tail;
	uint8_t count;
	uint8_t size;
#if(!SYMMETRIC_QUEUE)
	uint8_t* buffer;
#else
	uint8_t buffer[QUEUE_SIZE];
#endif
};

/**
 * Initialize the Queue before using it.
 */
#if(SYMMETRIC_QUEUE)
void queueInit(struct Queue* queue);
#else
void queueInit(struct Queue* queue, uint8_t size);
#endif
/**
 * Enqueues the provided data into the queue
 */
void enqueue(struct Queue* queue, uint8_t data);

/**
 * Dequeues the provided data from the queue
 */
uint8_t dequeue(struct Queue* queue);

/**
 * Peeks into the last inserted item of the queue
 */
uint8_t peekQueueTail(struct Queue* queue);

/**
 * Peaks into the first inserted item of the queue without dequeuing
 */
uint8_t peekQueueHead(struct Queue* queue);

#endif /* QUEUE_H_ */
