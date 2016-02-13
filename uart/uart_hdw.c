/*
 * uart_hdw.c
 *
 *  Created on: Feb 10, 2016
 *      Author: Aanal
 */

#include "uart_hdw.h"

/**
 * Check if the UART hardware is busy
 * returns an uint8_t that determines whether both rx and tx are busy or not
 */
uint8_t hdwIsBusyUART() {
	uint8_t result = 0x00;
	if (!(UCSRA & (1 << UDRE))) {
		//UDR is not empty and tx is busy
		result |= TX_BUSY;
	}
	if (!(UCSRA & (1 << RXC))) {
		//data not received yet
		result |= RX_BUSY;
	}
	return result;
}

/**
 * Interrupt driven design does not need to wait
 */
#if (INTERRUPT_DRIVEN && USE_QUEUE)

/**
 * Transmit data directly on the hardware
 */
inline void hdwTransmitUART(uint8_t data){
	UDR = data;
}

/**
 * Receive data directly from the hardware
 */
inline uint8_t hdwReceiveUART(void) {
	return UDR;
}

#else
/**
 * Transmit from the hardware
 */
void hdwTransmitUART(uint8_t data) {
	//wait for the transmitter to be ready
	while (!(UCSRA & (1 << UDRE)))
		;
	UDR = data;
}

/**
 * Receive from the hardware
 */
uint8_t hdwReceiveUART(void) {
	//wait for data to be received
	while (!(UCSRA & (1 << RXC)))
		;
	return UDR;
}

#endif
