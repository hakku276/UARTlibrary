/*
 * uart_hdw.c
 *
 *  Created on: Feb 10, 2016
 *      Author: Aanal
 */

#include "uart_hdw.h"

uint8_t hardwareStatus;

/**
 * Setup the UART hardware
 */
void hdwUARTSetup() {
	//setup Baud rate
	UBRRH = UBRR_VAL >> 8;
	UBRRL = UBRR_VAL;

	//setup the rx and tx pin
	UCSRB |= (1 << RXEN | 1 << TXEN);

	//setup parity to disabled and stop bits to 1
	UCSRC &= ~(1 << UPM1 | 1 << UPM0 | 1 << USBS);

	//setup frame size to 8 bits
	UCSRB &= ~(1 << UCSZ2);
	UCSRC |= (1 << UCSZ1 | 1 << UCSZ0);

	//enable interrupt driven architecture if required
#if INTERRUPT_DRIVEN
	//enable interrupt
	UCSRB |= 1 << RXCIE | 1 << TXCIE;

	hardwareStatus = 0x00;
#endif
}

/**
 * Check if the UART hardware is busy
 * returns an uint8_t that determines whether both rx and tx are busy or not
 */
uint8_t hdwIsBusyUART() {
	uint8_t result = 0x00;
	//TODO incorrect shit
	if(hardwareStatus & HDW_STATUS_TRANSMITTING){
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
inline void hdwTransmitUART(uint8_t data) {
	if(!(hardwareStatus & HDW_STATUS_TRANSMITTING)){
		hardwareStatus |= HDW_STATUS_TRANSMITTING;
	}
	if(UCSRA & (1<<UDRE)){
		UDR = data;
	}
}

/**
 * Receive data directly from the hardware
 */
inline uint8_t hdwReceiveUART(void) {
	return UDR;
}

ISR(USART_RXC_vect) {

	//read the received data even though it might be lost
	uint8_t data = hdwReceiveUART();

#if USE_QUEUE

	uint8_t uartStatus = UARTstatus();

	//using queue so enqueue into queue
	if (!(uartStatus & RX_QUEUE_FULL)) {

		//enqueue data in every case
		enqueue(&rxQueue, data);
#if (!COMMAND_RESPONSE_MODEL)
		//command response mode is not implemented  but queue is used
		//so notify the user program that the queue is full
		uartStatus = UARTstatus();
		if(uartStatus & RX_QUEUE_FULL) {
			(*rxcHandler)();
		}
#endif

		//if this is command response model check for command endpoints
#if COMMAND_RESPONSE_MODEL
		//TODO supposition of no escape sequence key
		if (data == COM_END) {
			//the command ended invoke the standard message handler
			standardMessageHandler();
		}
#endif
	}
#if COMMAND_RESPONSE_MODEL
	else {
		//RX queue is full transmit WAIT command
		//try and transmit it until successful
		//enable interrupt so that transmission can occur as planned
		//first read data from UDR so as to clear RXC flag
		status |= COM_STATUS_PARTNER_WAITING;

		struct Command command;
		initCommand(&command);

		command.commandCode = COM_WAIT;
#if USE_COMMAND_NUMBERING
		addCommandData(&command, outCommandNumber);
		addCommandData(&command, incCommandNumber);
#endif
		transmitCommandForced(&command);

		if (data == COM_END) {
			//The command has ended whatsoever
			standardMessageHandler();
		}
#if USE_COMMAND_NUMBERING
		else{
			//a command was sent but nothing was done for it
			incCommandNumber++;
		}
#endif
	}
#endif
#else
	// not using queue
	(*rxcHandler)(data);
#endif
}

/**
 * Interrupt Service Routine for UART Data transmission complete
 */
ISR(USART_TXC_vect) {
	//enable interrupt for Data Register Empty
	hardwareStatus &= ~HDW_STATUS_TRANSMITTING;
#if COMMAND_RESPONSE_MODEL
	//TODO not wait until this command has been completely transmitted
	if(status & COM_STATUS_REQUEST_SELF_WAIT){
		//requested self wait
		if(peekQueueHead(&txQueue) == COM_END){
			//the currently waiting byte to be transmitted is COM_END
			//transmit this data and then stop transmission
			status &= ~ COM_STATUS_REQUEST_SELF_WAIT;
			status |= COM_STATUS_SELF_WAITING;
		}
		//enable transmission for both cases
		UCSRB |= 1 << UDRIE;
	}else if (!(status & COM_STATUS_SELF_WAITING)) {
		//if the communication stream has not sent wait signal
#endif
		UCSRB |= 1 << UDRIE;
#if COMMAND_RESPONSE_MODEL
	}
#endif
}

/**
 * Interrupt Service Routine for UDRE. write data only if the UDR is ready to receive data
 */
ISR(USART_UDRE_vect) {
#if USE_QUEUE
	uint8_t uartStatus = UARTstatus();
	//using queue so dequeue from queue
	if (!(uartStatus & TX_QUEUE_EMPTY)) {
		//there is data remaining to be transmitted
		hdwTransmitUART(dequeue(&txQueue));
	}
#else
	//not using queue so notify user that the rxc was completed
	(*txcHandler)();
#endif
	//disable UDR empty interrupt
	UCSRB &= ~(1 << UDRIE);
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
