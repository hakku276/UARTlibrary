/*
 * uart.c
 *
 *  Created on: Feb 9, 2016
 *      Author: Aanal
 */
#include "uart.h"

#if USE_QUEUE
typedef struct Queue Queue;
Queue rxQueue, txQueue;

#if COMMAND_RESPONSE_MODEL
/**
 * NOTE: short reminder
 * void *(*ptr)();
 * ptr = &function;
 * (*ptr)();
 */
void *(*handler)(uint8_t);
#endif

#endif

/**
 * Simple UART Setup:
 * baud rate : BAUD_RATE
 * Parity: Disabled
 * Stop bits: 1
 * Frame Size: 8 bits
 */
void UARTsetup(
#if COMMAND_RESPONSE_MODEL
		void *(*messageHandler)(uint8_t)
#endif
		) {
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

	//setup queue if queue is to be used
#if USE_QUEUE
	queueInit(&rxQueue);
	queueInit(&txQueue);

	//set the message handler if required
#if COMMAND_RESPONSE_MODEL
	handler = messageHandler;
#endif

	//enable interrupt driven architecture if required
#if INTERRUPT_DRIVEN
	//enable interrupt
	UCSRB |= 1 << RXCIE | 1 << TXCIE;
#endif

#endif
}

/**
 * Advanced UART setup
 */
void advancedUARTsetup(
#if COMMAND_RESPONSE_MODEL
		void *(*messageHandler)(uint8_t)
#endif
		) {
	//TODO implement this
}

/**
 * The following code base is implemented if a Queue is to be implemented
 */
#if (USE_QUEUE)

/**
 * Check if the UART is busy (Software implementation)
 * returns an uint8_t that determines whether both rx and tx are busy or not
 */
uint8_t UARTstatus() {
	uint8_t result = 0;
	result = hdwIsBusyUART();
	//now check software status
	if (txQueue.count == 0) {
		result |= TX_QUEUE_EMPTY;
	} else if (txQueue.size == txQueue.count) {
		result |= TX_QUEUE_FULL;
	}
	if (rxQueue.count == 0) {
		result |= RX_QUEUE_EMPTY;
	} else if (rxQueue.size == rxQueue.count) {
		//rx queue is full
		result |= RX_QUEUE_FULL;
	}
	return result;
}

/**
 * Enqueues a byte of data for transmission into the UART stream
 */
uint8_t UARTtransmit(uint8_t data) {
	uint8_t uartStatus = UARTstatus();
	if (!(uartStatus & TX_QUEUE_FULL)) {
		//tx queue is not full
		enqueue(&txQueue, data);
		if (!(uartStatus & TX_BUSY)) {
			//tx is not busy
			hdwTransmitUART(dequeue(&txQueue));
		}
		//to denote that 1 byte was enqueued for writing
		return 1;
	}
	//to denote that 0 bytes were enqueued for writing
	return 0;
}

/**
 * Dequeues a byte of data from the UART received queue
 * It is wise to first check whether the Receive Queue is empty or not before gathering data
 * Use: UARTisBusy();
 */
uint8_t UARTreceive() {
	return dequeue(&rxQueue);
}

/**
 * Bulk transmit data into UART stream
 * If the length is greater than the available queue size the data is not queued up
 * and the number of bytes written is returned
 */
uint8_t UARTbulkTransmit(uint8_t* data, uint8_t start, uint8_t length) {
	uint8_t size = txQueue.size - txQueue.count;
	//check if the queue size is greater or smaller than the length of data
	if (size > length) {
		//the size of queue is greater, soo, we limit the size
		size = length;
	}
	//enqueue the data into the buffer from start till size
	for (uint8_t i = start; i < (start + size); i++) {
		enqueue(&txQueue, data[i]);
	}
	//check if there is data to be transmitted and whether the data is already being transmitted or not
	if ((!(UARTstatus() & TX_BUSY)) && (size > 0)) {
		//tx not initiated and there is data to be transmitted start transmission
		hdwTransmitUART(dequeue(&txQueue));
	}
	return size;
}

/**
 * Interrupt Driven Architecture
 */
#if INTERRUPT_DRIVEN

/**
 * Command Response Model should use a default message handler
 */
#if COMMAND_RESPONSE_MODEL

/**
 * Holds a standard UART command that will be taken into consideration before the next transmission
 */
uint8_t uartChainCommand;

/**
 * Incoming and Outgoing command number
 */
uint8_t incBlockNum, outBlockNum;

void UARTholdTransmit(){
	uartChainCommand = COM_WAIT;
	UARTprocessStatusNotification(PROC_STATUS_COMPLETED);
}


/**
 * Master mode command controller
 */
#if !MODE_SLAVE

/**
 * Determines the status of the process
 */
uint8_t processStatus;

#endif

#endif

/**
 * Interrupt Service Routine for UART reception complete
 */
ISR(USART_RXC_vect) {
	uint8_t uartStatus = UARTstatus();
	if (!(uartStatus & RX_QUEUE_FULL)) {
		//UARTqueue is not full
		uint8_t data = hdwReceiveUART();
		//enqueue data in every case
		enqueue(&rxQueue, data);
		//if the command has ended process it
		if(data != COM_END){
			//TODO implement this
		}
	}
#if COMMAND_RESPONSE_MODEL
	else {
		//RX queue is full transmit WAIT command
		//try and transmit it until successful
		//enable interrupt so that transmission can occur as planned
		//first read data from UDR so as to clear RXC flag
		//TODO if the input was end of command transmit wait but process data
		hdwReceiveUART();
		sei();
		while (!(UARTtransmit(COM_WAIT) == 1))
			;
	}
#endif
}

/**
 * Interrupt Service Routine for UART Data transmission complete
 */
ISR(USART_TXC_vect) {
	//enable interrupt for Data Register Empty
	if(uartChainCommand != COM_WAIT){
		//if the communication stream has not sent wait signal
		UCSRB |= 1 << UDRIE;
	}
}

/**
 * Interrupt Service Routine for UDRE. write data only if the UDR is ready to receive data
 */
ISR(USART_UDRE_vect) {
	uint8_t uartStatus = UARTstatus();
	if (!(uartStatus & TX_QUEUE_EMPTY)) {
		//there is data remaining to be transmitted
		hdwTransmitUART(dequeue(&txQueue));
	}
	//disable UDR empty interrupt
	UCSRB &= ~(1 << UDRIE);
}

/**
 * Interrupt not to be used but Queuing should be implemented
 */
#else
//TODO implement non interrupted queuing
#endif

/**
 * Not using UART Queue
 */
#else
/**
 * Checks whether the UART is busy or not
 */
uint8_t UARTstatus() {
	return hdwIsBusyUART();
}

/**
 * Transmits a byte of data into the data stream
 */
void UARTtransmit(uint8_t data) {
	hdwTransmitUART(data);
}

/**
 * Receives a byte of data from the data stream
 * waits until the data is available
 */
uint8_t UARTreceive(void) {
	return hdwReceiveUART();
}
#endif
