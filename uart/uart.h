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

#if (USE_QUEUE)
#include "../utils/Queue.h"

#if (INTERRUPT_DRIVEN)
#include <avr/interrupt.h>
#endif

#if COMMAND_RESPONSE_MODEL
#include "../commands.h"

#if (!(defined(COM_END) && defined(COM_WAIT)))
#error 'Required Commands not defined for Command Oriented Communication'
#endif

#endif

#endif


/**
 * Requires a message handler if COMMAND RESPONSE MODEL has been implemented
 */
#if COMMAND_RESPONSE_MODEL

void UARTholdTransmit();

/**
 * simple UART setup
 */
void UARTsetup(void*(*messageHandler)(uint8_t));

/**
 * Advanced UART setup
 */
void advancedUARTsetup(void*(*messageHandler)(uint8_t));

#else

/**
 * simple UART setup
 */
void UARTsetup();

/**
 * Advanced UART setup
 */
void advancedUARTsetup();

#endif

/**
 * Transmit data (enqueue to buffer or direct) according to use cases
 * Returns the number of bytes written into the stream
 */
uint8_t UARTtransmit(uint8_t);

/**
 * Receive data (from buffer or direct) according to use cases
 */
uint8_t UARTreceive();

/**
 * Check whether UART is busy or not. a software implementation
 * returns a uint8_t value that represents both transmit and receive business
 */
uint8_t UARTstatus();

/**
 * Bulk Transmit data into the stream.
 * start denotes the start point of the array
 * length denotes the number of data bytes to transmit
 */
uint8_t UARTbulkTransmit(uint8_t* data, uint8_t start, uint8_t length);

#endif /* UART_H_ */
