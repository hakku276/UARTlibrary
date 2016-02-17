/*
 * uart.c
 *
 *  Created on: Feb 9, 2016
 *      Author: Aanal
 */
#include "uart.h"

#if COMMAND_RESPONSE_MODEL
#include "../commands.h"
#endif

#if COMMAND_RESPONSE_MODEL
/**
 * A custom message handler
 */
void (*handler)(struct Command*);
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
		) {
	//setup hardware first
	hdwUARTSetup();

	//setup queue if queue is to be used
#if USE_QUEUE
	queueInit(&rxQueue);
	queueInit(&txQueue);

	//set the message handler if required
#if COMMAND_RESPONSE_MODEL
	status = 0;
	handler = messageHandler;

#if USE_COMMAND_NUMBERING
	incCommandNumber = 0;
	outCommandNumber = 0;
#endif
#endif
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
		void (*messageHandler)(struct Command*)
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
void UARTbeginTransmit() {
	uint8_t status = UARTstatus();
	if ((!(status & TX_BUSY)) && (txQueue.count != 0)) {
		//the transmitter is not busy and there is data to be transmitted
		hdwTransmitUART(dequeue(&txQueue));
	}
}

/**
 * Builds the queue but does not transmit.
 * Returns whether the data was written or not
 */
uint8_t UARTbuildTransmitQueue(uint8_t data) {
	if (txQueue.count != txQueue.size) {
		//tx queue is not full
		enqueue(&txQueue, data);
		return 1;
	}
	return 0;
}

/**
 * Command Response Model should use a default message handler
 */
#if COMMAND_RESPONSE_MODEL

/**
 * First level standard messages handler
 * Do not forget to clear the com_end after every standard command
 */
void standardMessageHandler() {
	uint8_t code = UARTreceive();
	struct Command command;
	initCommand(&command);

#if USE_COMMAND_NUMBERING
	uint8_t messageNumber = UARTreceive();

	//verify message number first
	if ((messageNumber != incCommandNumber)
			&& (code != COM_RESYNC_COMMAND_NUMBER)) {
		//there was a mismatch in message validation and the message was not fur a resync
		//reply with resync number
		command.commandCode = COM_RESYNC_COMMAND_NUMBER;
		addCommandData(&command, outCommandNumber);
		addCommandData(&command, incCommandNumber);
		transmitCommand(&command);
		return;
	}
#endif

	//then start processing data
	switch (code) {
	case COM_WAIT:
		status |= COM_STATUS_REQUEST_SELF_WAIT;
		command.commandCode = COM_ACK;
#if USE_COMMAND_NUMBERING
		addCommandData(&command, outCommandNumber);
#endif
		transmitCommand(&command);
		//dequeue com_end from receive queue
		UARTreceive();
		break;
		//TODO restart from the begining of this command not where it has been left or stop after a command is sent
	case COM_RESUME:
		//this device can't handle resume with some number but, we receive whichever number it has sent
		//and then start our transmission
		UARTbeginTransmit();
		//dequeue com_end from receive queue
		UARTreceive();
		break;
		//TODO handle COM_ACK
	case COM_ACK:
		if (status & COM_STATUS_WAITING_ACK) {
			//todo determine what was waiting for an ack and do something
		} else {
			// not waiting for an ack so forward it to the custom message handler
			command.commandCode = code;
			fillIncomingData(&command);
			(*handler)(&command);
		}
		break;
	case COM_RESYNC_COMMAND_NUMBER:
		incCommandNumber = messageNumber;
		outCommandNumber = UARTreceive();
		//clear com_end from the queue
		UARTreceive();
		break;
	default:
		command.commandCode = code;
		fillIncomingData(&command);
		(*handler)(&command);
		break;
	}
	notify(PROC_STATUS_COMPLETED);
}

/**
 * Fills data into the command structure from the rx queue
 * TODO verify data integrity using escape sequencing
 */
void fillIncomingData(struct Command* command) {
	uint8_t data = UARTreceive();
	command->dataSize = 0;
	//TODO use ESCAPE SEQUENCE
	while ((data != COM_END) && (command->dataSize < COMMAND_DATA_LENGTH)) {
		command->data[command->dataSize] = data;
		data = UARTreceive();
		command->dataSize++;
	}
}

/**
 * Notify the process status of UART.
 */
void notify(uint8_t processStatus) {
	if (processStatus == PROC_STATUS_COMPLETED) {
		//incoming command number processing complete
#if USE_COMMAND_NUMBERING
		incCommandNumber++;
#endif
		//todo what to do at overflow
		//the process status completed successfully
		if (status & COM_STATUS_PARTNER_WAITING) {
			struct Command command;
			initCommand(&command);
			//if the communication channel is waiting request resume
			command.commandCode = COM_RESUME;
#if USE_COMMAND_NUMBERING
			addCommandData(&command, outCommandNumber);
			addCommandData(&command, incCommandNumber);
#endif
			transmitCommandForced(&command);
			//TODO how to wait for an ack for this?
			//using timers??
			UARTbeginTransmit();
			status &= ~COM_STATUS_PARTNER_WAITING;
		}
	}
}

/**
 * Master mode command controller
 */
#if !MODE_SLAVE

/**
 * Determines the status of the process
 */
uint8_t processStatus;

void UARTholdTransmit() {
	status = COM_WAIT;
}

void UARTresumeTransmission() {
	UARTbeginTransmit();
}

#endif

#endif

#endif
