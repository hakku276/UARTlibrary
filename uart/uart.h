/*
 * uart.h
 *
 *  Created on: Feb 9, 2016
 *      Author: Aanal
 */

#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <inttypes.h>
#include "config.h"
#include "uart_hdw.h"

#if (INTERRUPT_DRIVEN)

/**
 * Global Variables required for UART when using interrupt mode
 */
#if COMMAND_RESPONSE_MODEL

#include "../commands.h"

#include "../utils/commandBuilder.h"

//check settings for command response model
#if !(defined(COM_END) && defined(COM_WAIT))
#error 'Required Commands not defined for Command Oriented Communication'
#endif

/**
 * Holds a standard UART command that will be taken into consideration before the next transmission
 */
uint8_t status;

#endif

#if USE_COMMAND_NUMBERING
/**
 * Holds the incoming command number
 */
uint8_t incCommandNumber;

/**
 * Holds the outgoing commad number
 */
uint8_t outCommandNumber;

#endif

#endif

#if USE_QUEUE
#include "../utils/Queue.h"

typedef struct Queue Queue;
Queue rxQueue, txQueue;

#if COMMAND_RESPONSE_MODEL
#endif
#else

//not using queue so command response model is not valid
#if COMMAND_RESPONSE_MODEL
//command response model used without using queue
#error 'Invalid Setting (Queue: 0 and CommandModel: 1)'
#endif

#endif

/**
 * simple UART setup
 * NOTE: the message handler should not dequeue the command end from the device
 */
void UARTsetup(
#if COMMAND_RESPONSE_MODEL
		void (*messageHandler)(struct Command*)
#endif
#if (INTERRUPT_DRIVEN && (!USE_QUEUE))
		void (*rxcCompleteHandler)(uint8_t),
		void (*txcCompleteHandler)()
#endif
#if (INTERRUPT_DRIVEN && (USE_QUEUE && (!COMMAND_RESPONSE_MODEL)))
		void (*rxcQueueFullHandler)(),
		void (*txcCompleteHandler)()
#endif
		);

/**
 * Advanced UART setup
 */
void advancedUARTsetup(
#if COMMAND_RESPONSE_MODEL
		void (*messageHandler)(struct Command*)
#endif
#if (INTERRUPT_DRIVEN && (!USE_QUEUE))
		void (*rxcCompleteHandler)(uint8_t),
		void (*txcCompleteHandler)()
#endif
#if (INTERRUPT_DRIVEN && (USE_QUEUE && (!COMMAND_RESPONSE_MODEL)))
		void (*rxcQueueFullHandler)(),
		void (*txcCompleteHandler)()
#endif
		);

/**
 * Check whether UART is busy or not. a software implementation
 * returns a uint8_t value that represents both transmit and receive business
 */
uint8_t UARTstatus();

/**
 * Transmit data (enqueue to buffer or direct) according to use cases
 * Returns the number of bytes written into the stream
 */
uint8_t UARTtransmit(uint8_t);

/**
 * Receive data (from buffer or direct) according to use cases
 */
uint8_t UARTreceive();

#if USE_QUEUE

/**
 * Initiate transmission the data from the queue.
 */
void UARTbeginTransmit();

/**
 * Builds the queue but does not transmit.
 * Returns whether the data was written or not
 */
uint8_t UARTbuildTransmitQueue(uint8_t data);

/**
 * Bulk Transmit data into the stream.
 * start denotes the start point of the array
 * length denotes the number of data bytes to transmit
 * TODO make this common
 */
uint8_t UARTbulkTransmit(uint8_t* data, uint8_t start, uint8_t length);

#endif

#if COMMAND_RESPONSE_MODEL

/**
 * Resumes the current transmission queue
 */
void UARTresumeTransmission();

/**
 * Pauses the Current transmission queue
 */
void UARTholdTransmit();

/**
 * Fills data into the provided command data structure
 */
void fillIncomingData(struct Command* command);

/**
 * Defines a Standard message handler
 */
void standardMessageHandler();

/**
 * Notifies the status of the message handling mechanism
 */
void notify(uint8_t);

#endif

#endif /* UART_H_ */
