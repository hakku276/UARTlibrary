/*
 * commandBuilder.c
 *
 *  Created on: Feb 15, 2016
 *      Author: Aanal
 */

#include "commandBuilder.h"

void initCommand(struct Command* command) {
	command->commandCode = 0;
	command->dataSize = 0;
}

/**
 * Sets the provided data array as the command data.
 * Overwrites any previous data
 */
void setCommandData(struct Command* command, uint8_t* data, uint8_t size) {
	//TODO implement this
}

/**
 * Appends the data byte into the command structure and returns the remaining
 * size of the structure
 */
uint8_t addCommandData(struct Command* command, uint8_t data) {
	//TODO implement escape sequencing
	if (command->dataSize < COMMAND_DATA_LENGTH) {
		command->data[command->dataSize] = data;
		command->dataSize++;
		return (COMMAND_DATA_LENGTH - command->dataSize);
	}
	return 0;
}

/**
 * Queues the command for transmission
 */
void transmitCommand(struct Command* command) {
	UARTbuildTransmitQueue(command->commandCode);
	for (uint8_t i = 0; i < command->dataSize; i++) {
		UARTbuildTransmitQueue(command->data[i]);
	}
	UARTbuildTransmitQueue(COM_END);
	UARTbeginTransmit();
}

/**
 * Queues the command for transmission and makes sure it happens
 */
void transmitCommandForced(struct Command* command) {
	while (!(UARTtransmit(command->commandCode) == 1))
		;
	for (uint8_t i = 0; i < command->dataSize; i++) {
		while (!(UARTtransmit(command->data[i]) == 1))
			;
	}
	while (!(UARTtransmit(COM_END) == 1))
		;
}
