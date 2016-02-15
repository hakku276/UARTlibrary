/*
 * commandBuilder.h
 *
 *  Created on: Feb 15, 2016
 *      Author: Aanal
 */

#ifndef COMMANDBUILDER_H_
#define COMMANDBUILDER_H_

#include "../commands.h"
#include "inttypes.h"
#include "../uart/uart.h"

struct Command {
	/**
	 * The command code
	 */
	uint8_t commandCode;
	/**
	 * The maximum size the data can be
	 */
	uint8_t data[COMMAND_DATA_LENGTH];
	/**
	 * The size of this current data is
	 */
	uint8_t dataSize;
};

/**
 * Initialises the command structure
 */
void initCommand(struct Command* command);

/**
 * Sets the provided data array as the command data.
 * Overwrites any previous data
 */
void setCommandData(struct Command* command,uint8_t* data, uint8_t size);

/**
 * Appends the data byte into the command structure and returns the remaining
 * size of the structure
 */
uint8_t addCommandData(struct Command* command, uint8_t data);

/**
 * Forwards the Command into the transmit queue
 */
void transmitCommand(struct Command* command);

/**
 * Makes sure the command is transmitted
 */
void transmitCommandForced(struct Command* command);

#endif /* COMMANDBUILDER_H_ */
