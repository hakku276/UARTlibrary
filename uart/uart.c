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
void (*handler)(uint8_t);
#endif

#endif

/**
 * Require callbacks if not using queues
 */
#if (INTERRUPT_DRIVEN && (!USE_QUEUE))
		void (*rxcHandler)(uint8_t);
		void (*txcHandler)();
#endif
#if (INTERRUPT_DRIVEN && (USE_QUEUE && (!COMMAND_RESPONSE_MODEL)))
		void (*rxcHandler)();
		void (*txcHandler)();
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
		void (*messageHandler)(uint8_t)
#endif
#if (INTERRUPT_DRIVEN && (!USE_QUEUE))
		void (*rxcCompleteHandler)(uint8_t),
		void (*txcCompleteHandler)()
#endif
#if (INTERRUPT_DRIVEN && (USE_QUEUE && (!COMMAND_RESPONSE_MODEL)))
		void (*rxcQueueFullHandler)(),
		void (*txcCompleteHandler)()
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

#endif
	//enable interrupt driven architecture if required
#if INTERRUPT_DRIVEN
	//enable interrupt
	UCSRB |= 1 << RXCIE | 1 << TXCIE;
#endif

	//enable callback methods if there is interrupt enabled but not queuing is used
	//or if queue is used but not command response model
#if (INTERRUPT_DRIVEN && (!USE_QUEUE))
	rxcHandler = rxcCompleteHandler;
	txcHandler = txcCompleteHandler;
#endif
#if (INTERRUPT_DRIVEN && (USE_QUEUE && (!COMMAND_RESPONSE_MODEL)))
	rxcHandler = rxcQueueFullHandler;
	txcHandler = txcCompleteHandler;
#endif
}

/**
 * Advanced UART setup
 */
void advancedUARTsetup(
#if COMMAND_RESPONSE_MODEL
		void (*messageHandler)(uint8_t)
#endif
#if (INTERRUPT_DRIVEN && (!USE_QUEUE))
		void (*rxcCompleteHandler)(uint8_t),
		void (*txcCompleteHandler)()
#endif
#if (INTERRUPT_DRIVEN && (USE_QUEUE && (!COMMAND_RESPONSE_MODEL)))
		void (*rxcCompleteHandler)(),
		void (*txcCompleteHandler)()
#endif
		) {
	//TODO implement this
}

/**
 * Check if the UART is busy (Software implementation)
 * returns an uint8_t that determines whether both rx and tx are busy or not
 */
uint8_t UARTstatus() {
	uint8_t result = 0;

	//check hardware status
	result = hdwIsBusyUART();

#if USE_QUEUE

	//now check queue status
	//check tx queue is empty or full
	if (txQueue.count == 0) {
		result |= TX_QUEUE_EMPTY;
	} else if (txQueue.size == txQueue.count) {
		result |= TX_QUEUE_FULL;
	}

	//check rx queue is empty or full
	if (rxQueue.count == 0) {
		result |= RX_QUEUE_EMPTY;
	} else if (rxQueue.size == rxQueue.count) {
		result |= RX_QUEUE_FULL;
	}

#endif

	return result;
}

/**
 * Enqueues a byte of data for transmission into the UART stream
 */
uint8_t UARTtransmit(uint8_t data) {
#if USE_QUEUE
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
#else
	hdwTransmitUART(data);
	return 1;
#endif
}

/**
 * Dequeues a byte of data from the UART received queue
 * It is wise to first check whether the Receive Queue is empty or not before gathering data
 * Use: UARTisBusy();
 */
uint8_t UARTreceive() {
#if USE_QUEUE
	return dequeue(&rxQueue);
#else
	return hdwReceiveUART();
#endif
}

/**
 * ------------------------------------------------------
 * Interrupt Service Routine for UART reception complete
 * ------------------------------------------------------
 */
#if INTERRUPT_DRIVEN

/**
 * Global Variables required for UART
 */
#if COMMAND_RESPONSE_MODEL
/**
 * Holds a standard UART command that will be taken into consideration before the next transmission
 */
uint8_t uartChainCommand;
#endif

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
		if(uartStatus & RX_QUEUE_FULL){
			(*rxcHandler)();
		}
#endif

		//if this is command response model check for command endpoints
#if COMMAND_RESPONSE_MODEL
		if(data == COM_END){
			//the command ended!!!
			(*handler)(dequeue(&rxQueue));
		}
#endif
	}
#if COMMAND_RESPONSE_MODEL
	else {
		//RX queue is full transmit WAIT command
		//try and transmit it until successful
		//enable interrupt so that transmission can occur as planned
		//first read data from UDR so as to clear RXC flag
		//TODO if the input was end of command transmit wait but process data
		sei();
		while (!(UARTtransmit(COM_WAIT) == 1))
			;
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
#if COMMAND_RESPONSE_MODEL
	if(uartChainCommand != COM_WAIT){
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
#endif

/**
 * The following code base is implemented if a Queue is to be implemented
 */
#if (USE_QUEUE)

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
 * Initiate transmission the data from the queue.
 */
void UARTbeginTransmit(){
	uint8_t status = UARTstatus();
	if((!(status&TX_BUSY)) && (txQueue.count != 0)){
		//the transmitter is not busy and there is data to be transmitted
		hdwTransmitUART(dequeue(&txQueue));
	}
}

/**
 * Builds the queue but does not transmit.
 * Returns whether the data was written or not
 */
uint8_t UARTbuildCommandQueue(uint8_t data){
	if(txQueue.count != txQueue.size){
		//tx queue is not full
		enqueue(&txQueue,data);
		return 1;
	}
	return 0;
}


/**
 * Command Response Model should use a default message handler
 */
#if COMMAND_RESPONSE_MODEL

/**
 * Master mode command controller
 */
#if !MODE_SLAVE

/**
 * Determines the status of the process
 */
uint8_t processStatus;

void UARTholdTransmit(){
	uartChainCommand = COM_WAIT;
}

void UARTresumeTransmission(){
	UARTbeginTransmit();
}

#endif

#endif

#endif
