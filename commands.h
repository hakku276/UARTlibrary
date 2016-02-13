/*
 * commands.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Aanal
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

/**
 * ---------------------------
 * PERSONAL COMMANDS SECTION
 * ---------------------------
 */

/**
 * Verifies whether the byte is a custom command code or not
 */
/*
uint8_t isCustomCommand(uint8_t data) {
	return 0x00;
}*/

/**
 * ---------------------------
 * REQUIRED COMMANDS SECTION
 * ---------------------------
 */
#define COM_WAIT 0x01
#define COM_END 0x02
#define COM_ESCAPE_CHAR 0x03

/**
 * Verifies whether the byte is a standard command code or not
 */
/*
uint8_t isStandardCommand(uint8_t data) {
	return ((data == COM_WAIT) || (data == COM_END) || (data == COM_ESCAPE_CHAR));
}*/

/**
 * Verifies whether the byte is a command code or not
 */
/*
uint8_t isCommandCode(uint8_t data) {
	return (isCustomCommand(data) || isStandardCommand(data));
}*/
#endif /* COMMANDS_H_ */
