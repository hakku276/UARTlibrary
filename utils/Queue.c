/*
 * Queue.c
 *
 *  Created on: Feb 9, 2016
 *      Author: Aanal
 */

#include "../utils/Queue.h"
#if(!SYMMETRIC_QUEUE)
#include <alloca.h>
#endif

#if(SYMMETRIC_QUEUE)
void queueInit(struct Queue* queue){
	queue->head = 0;
	queue->tail = 0;
	queue->count = 0;
	queue->size = QUEUE_SIZE;
}
#else
void queueInit(struct Queue* queue, uint8_t size){
	queue->buffer = (uint8_t*) malloc(size);
	queue->head = 0;
	queue->size = size;
	queue->tail = 1;
	queue->count = 0;
}
#endif

void enqueue(struct Queue* queue, uint8_t data){
#if(!SYMMETRIC_QUEUE)
	if(queue->count!=queue->size){
#else
	if(queue->count != QUEUE_SIZE){
#endif
		//queue is not full
		queue->buffer[queue->tail] = data;
		//increment the tail and the count
		queue->count++;
		queue->tail++;
		//check if the tail is out of array
#if(!SYMMETRIC_QUEUE)
		if(queue->tail > queue->size){
#else
		if(queue->tail == QUEUE_SIZE){
#endif
			//the tail is out of the array
			queue->tail = 0;
		}
	}
}

uint8_t dequeue(struct Queue* queue){
	uint8_t data = 0;
	if(queue->count != 0){
		//the queue is not empty
		data = queue->buffer[queue->head];
		//increment header
		queue->head++;
		//decrement count
		queue->count--;
		//check if header is out of the array
#if(!SYMMETRIC_QUEUE)
		if(queue->head > queue->size){
#else
		if(queue->head == QUEUE_SIZE){
#endif
			//the tail is out of the array
			queue->head = 0;
		}
	}
	return data;
}


inline uint8_t peekQueueTail(struct Queue* queue){
	return queue->buffer[queue->tail];
}

inline uint8_t peekQueueHead(struct Queue* queue){
	return queue->buffer[queue->head];
}
