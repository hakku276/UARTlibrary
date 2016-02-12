/*
 * uart_hdw.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Aanal
 */

#ifndef UART_HDW_H_
#define UART_HDW_H_
#include <avr/io.h>
#include "config.h"

/**
 * Checks if the hardware is busy or not
 */
uint8_t hdwIsBusyUART();

/**
 * Transmit 8 bit data into the UART hardware stream.
 */
void hdwTransmitUART(uint8_t);

/**
 * Read received data from the hardware direct
 */
uint8_t hdwReceiveUART(void);


#endif /* UART_HDW_H_ */
