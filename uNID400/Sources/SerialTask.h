/*
 * SerialTask.h
 *
 *  Created on: Dec 26, 2016
 *      Author: Larry
 */

#ifndef SOURCES_SERIALTASK_H_
#define SOURCES_SERIALTASK_H_

#define MAX_INPUT_LENGTH	520
#define BROADCAST_ADDRESS	0xf0

#define MODEL_YES			1
#define MODEL_NO			0

//commands
#define MODEL_DESCRIPTION 8
#define PROGRAM_CODE	9

#define PROGRAM_START		0xAA;

//address state
enum{
	ADDR_NOT_OURS = 0,
	ADDR_IS_LOCAL,
	ADDR_IS_BROADCAST
};
//FW update states
enum{
	STATE_NOT_STARTED = 0,
	STATE_DESCRIPTION,
	STATE_START,
	STATE_START_PROGRAM,
	STATE_PROGRAM_COMPLETE,
	STATE_SEND_NEXT,
	STATE_PROGRAM_ERROR
};

// possible errors
#define SREC_OK					0
#define SREC_ERR_CHAR           0x80
#define SREC_ERR_RECORD         0x81
#define SREC_ERR_SHORT          0x82
#define SREC_ERR_CHECKSUM       0x83
#define SREC_ERR_START          0x84
#define SREC_ERR_END            0x85
#define SREC_ERR_EOF            0x86
#define SREC_ERR_ERROR          0x87
#define SREC_ERR_NOTUB          0x88
#define SREC_ERR_WRITE			0x89

#endif /* SOURCES_SERIALTASK_H_ */
