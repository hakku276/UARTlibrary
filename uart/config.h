/*
 * commconfig.h
 *
 *  Created on: Feb 9, 2016
 *      Author: Aanal
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/**
 * Define whether the queues created should be of equal length
 * 0 - Queues have different length
 * 1 - Queues have same length
 */
#define SYMMETRIC_QUEUE 1

/**
 * Define whether the queuing system should be used or not
 */
#define USE_QUEUE 1

/**
 * Define whether the uart system should be interrupt driven
 * Note: Useful only when Queuing is implemented
 */
#define INTERRUPT_DRIVEN 1

/**
 * Define whether the UART system should be follow a command response model as a slave.
 * The commands are specified in "commands.h" in the root folder and must specify some
 * specific constants.
 */
#define COMMAND_RESPONSE_MODEL 1

/**
 * ONLY SLAVE SUPPORTED TILL NOW
 * TODO implement master as well
 */
#define MODE_SLAVE 1

/**
 * If the queue is supposed to be equal for every type, then define the queue size.
 */
#if(SYMMETRIC_QUEUE)
#define QUEUE_SIZE 10
#endif

//Communication settings
#define BAUD_RATE 9600U

#if (!defined(BAUD_RATE))
#error 'BAUD Rate should be defined'
#else
#define UBRR_VAL (uint16_t)((F_CPU/16)/BAUD_RATE-1)
#endif

/**
 * -------------------------------------------------------------------------
 * DO NOT MODIFY ANYTHING BELOW THIS IF YOU ARE UNCERTAIN ABOUT THE RESULTS.
 * -------------------------------------------------------------------------
 */

/**
 * The hardware transmitter cannot accept anything now
 */
#define TX_BUSY 0x01
/**
 * The hardware receiver has nothing to provide.
 */
#define RX_BUSY 0x02

#if USE_QUEUE

/**
 * The transmitter queue is currently full
 */
#define TX_QUEUE_FULL 0x04

/**
 * The transmitter queue is currently empty
 */
#define TX_QUEUE_EMPTY 0x08

/**
 * The receiver queue is currently empty
 */
#define RX_QUEUE_EMPTY 0x10

/**
 * The receiver queue is currently full
 */
#define RX_QUEUE_FULL 0x20

#endif

#if COMMAND_RESPONSE_MODEL

/**
 * The standard message handler is still processing the command stream
 */
#define PROC_STATUS_STD_PROCESSING 0x02

/**
 * The customer message handler is still processing the command stream
 */
#define PROC_STATUS_PROCESSING 0x03

/**
 * The message handler has completed a command stream and waiting to process another stream
 */
#define PROC_STATUS_COMPLETED 0x04

#endif

#endif /* CONFIG_H_ */
