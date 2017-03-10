/*
 * SerialTask.c
 *
 *  Created on: Dec 26, 2016
 *      Author: Larry
 */

#include "config.h"
#include <mqx.h>
#include <bsp.h>
#include "extern.h"
#include <string.h>
#include "serialtask.h"

static void Serial_RX_ISR( void* user_isr_ptr );
uint_32 SendToFlash(uint_16 Count, uint_8* data);

MQX_FILE_PTR serial_fd;
uint_8 InputMessage422[MAX_INPUT_LENGTH];
uint_8 OutputReply[10];
uint_16 readptrA, writeptrA, Checksum;
uint_8 ScreenState, IsLocal, SendCount;
uint_16 ProgramTimeout, ByteCount;
bool IsModel;
uint_32 res;
uint_8* LastAddress;

/* Firmware update states
 * 1) STATE_NOT_STARTED state
 * 	a) Controller sends either broadcast address or individual address (byte 0)
 * 	b) Controller sends command byte MODEL_DESCRIPTION (byte 1)
 * 	c) Controller sends length of description (byte 2 = 0, byte 3)
 * 	d) Controller sends description string (byte 4 - x)
 *
 * 	If individual address:
 * 	 a) Slave sends individual address (byte 0)
 * 	 b) Slave sends command byte MODEL_DESCRIPTION (byte 1)
 * 	 c) Slave sends length of 1 (byte 2 = 0, byte 3 = 1)
 * 	 d) Slave sends MODEL_YES or MODEL_NO back to controller (yes = desccription matches, no = it doesnt) (byte 4)
 * 	If individual or broadcast address:
 * 	  1) If MODEL_YES, sets IsModel bit, and enters STATE_START state
 * 	  2) If MODEL_NO, clears IsModel bit and remaining transactions  are ignored until next MODEL_DESCRIPTION command
 *
 * 2) STATE_START state
 * 	a) Controller sends either broadcast address or individual address (byte 0)
 * 	b) Controller sends command byte PROGRAM_CODE (byte 1)
 * 	c) Controller sends length of bytes (byte 2 (MSB), byte 3 (LSB))  (0 bytes)
 * 	If individual address:
 * 	 a) Slave sends individual address (byte 0)
 * 	 b) Slave sends command byte PROGRAM_CODE (byte 1)
 * 	 c) Slave sends length of 1  (byte 2 = 0, byte 3 = 1)
 * 	 d) Slave sends STATE_START_PROGRAM or STATE_PROGRAM_ERROR (byte 4)
 * 	  1) If error, returns to STATE_NOT_STARTED state (memory didnt clear)
 * 	  2) If no error, slave enters STATE_START_PROGRAM state
 *
 * 3) STATE_START_PROGRAM state
 * 	a) Controller sends either broadcast address or individual address (byte 0)
 * 	b) Controller sends command byte PROGRAM_CODE (byte 1)
 * 	c) Controller sends length of bytes (byte 2 (MSB), byte 3 (LSB))  (0 - 512 bytes + 2 checksum)
 * 	d) Controller sends bytes (byte 4 - x)
 * 	e) Controller sends 2 checksum bytes (byte x + 1, byte x + 2)
 * 	 a) Slave sends individual address (byte 0)
 * 	 b) Slave sends command byte PROGRAM_CODE (byte 1)
 * 	 c) Slave sends length of 1  (byte 2 = 0, byte 3 = 1)
 * 	 d) Slave sends STATE_SEND_NEXT or STATE_PROGRAM_ERROR (byte 4)
 * 	  1) If error, returns to STATE_NOT_STARTED state
 *
 * 	  NOTE: If byte count is less than 512, this is the last packet!
 * 	  If last packet and OK and individual address:
 * 	   a) Slave sends individual address (byte 0)
 * 	   b) Slave sends command byte PROGRAM_CODE (byte 1)
 * 	   c) Slave sends length of 1  (byte 2 = 0, byte 3 = 1)
 * 	   d) Slave sends STATE_PROGRAM_COMPLETE (byte 4)
 * 	  Slave copies code to program flas and reboots
 */

void SerialTask(uint32_t value){
	uint_32 error;
	UART_MemMapPtr sci_ptr = UART1_BASE_PTR;

	readptrA = writeptrA = 0;
	ByteCount = 0;
	serial_fd = 0;
	serial_fd =  fopen("ttyb:", 0);
	if (!serial_fd)
		_task_block();
	error = IO_SERIAL_RX_NON_BLOCKING;
	ioctl(serial_fd, IO_IOCTL_SERIAL_SET_FLAGS, &error);
	if (_int_install_isr(INT_UART1_RX_TX, Serial_RX_ISR, NULL) == 0)	//install the interrupt
		_task_block();
	if (_cortex_int_init(INT_UART1_RX_TX, 2, 0, FALSE) != MQX_OK)		//initialize the interrupt
		_task_block();
	sci_ptr->C2 = RCVINT_XMT_RCV_ENABLED;
	sci_ptr->RWFIFO = 1;				//set watermark to 1
	sci_ptr->CFIFO = FLUSH_XMT_RCV_BUFFER;
	ScreenState = STATE_NOT_STARTED;
	ProgramTimeout = 0;
	IsModel = false;
	IsLocal = ADDR_NOT_OURS;
	SendCount = 0;
	_cortex_int_enable(INT_UART1_RX_TX);				//enable the interrupt
	while(1){
		if (writeptrA > 3){													//if enough bytes to get the count...
			ProgramTimeout = 1000;											//just in case we dont get a full message...
			ByteCount = (InputMessage422[2] << 8) | InputMessage422[3];		//get the byte count
			if (writeptrA == (ByteCount + 4)){								//if entire message read in...
				if (InputMessage422[0] == UnitAddress)
					IsLocal = ADDR_IS_LOCAL;
				else if (InputMessage422[0] == BROADCAST_ADDRESS)
					IsLocal = ADDR_IS_BROADCAST;
				else{
					IsLocal = ADDR_NOT_OURS;
					writeptrA = 0;
					ScreenState = STATE_NOT_STARTED;
					ProgramTimeout = 0;
					continue;
				}
				if (InputMessage422[1] == MODEL_DESCRIPTION){				//if MODEL_DESCRIPTION command
					if (ScreenState == STATE_NOT_STARTED && ByteCount < 33){		//if not started and a valid length
						InputMessage422[ByteCount + 4] = 0;
						strncpy(RcvdProductString, &InputMessage422[4], ByteCount + 1);	//save the description
						if (strcmp(RcvdProductString, ProductString))		//make sure it is for us
							IsModel = false;
						else{
							IsModel = true;
							ScreenState = STATE_START;
						}
						if (IsLocal == ADDR_IS_LOCAL){
							OutputReply[0] = UnitAddress;
							OutputReply[1] = MODEL_DESCRIPTION;
							OutputReply[2] = 0;
							OutputReply[3] = 1;
							OutputReply[4] = IsModel;
							SendCount = 5;
							ProgramTimeout = 300;
						}
					}
					writeptrA = 0;
				}
				else if (IsModel && InputMessage422[1] == PROGRAM_CODE){
					if (ScreenState == STATE_START){
						if (ByteCount == 0){
							for (error = 0; error < 0x18000; error += 0x1000){			//erase bank 1 flash area
								fseek(flash_file, error, IO_SEEK_SET);
								if (_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL) != MQX_OK)
									break;
							}
							SRECLength = 0;
							LastAddress = SRECFilePtr;
							if (error == 0x18000)
								error = 0;
							if (IsLocal == ADDR_IS_LOCAL){
								OutputReply[0] = UnitAddress;
								OutputReply[1] = PROGRAM_CODE;
								OutputReply[2] = 0;
								OutputReply[3] = 1;
								if (error)
									OutputReply[4] = STATE_PROGRAM_ERROR;
								else
									OutputReply[4] = STATE_START_PROGRAM;
								SendCount = 5;
								ProgramTimeout = 30;
							}
							if (error){
								ScreenState = STATE_NOT_STARTED;
								ProgramTimeout = 0;
								IsModel = false;
							}
							else
								ScreenState = STATE_START_PROGRAM;
						}
					}
					else if (ScreenState == STATE_START_PROGRAM){
						OutputReply[0] = UnitAddress;
						OutputReply[1] = PROGRAM_CODE;
						OutputReply[2] = 0;
						OutputReply[3] = 1;
						if (ByteCount == 0){					//no bytes = done
							if (SRECLength == 0)							//if no data received
								OutputReply[4] = STATE_PROGRAM_ERROR;
							else{
								OutputReply[4] = STATE_PROGRAM_COMPLETE;	//OK we got it so reboot
								Reboot = true;
							}
							ScreenState = STATE_NOT_STARTED;
							IsModel = false;
						}
						else{												//if data...
							res = SendToFlash(ByteCount, (uint_8*)&InputMessage422[4]);	//verify it
							if (!res)										//if no errors...
								OutputReply[4] = STATE_SEND_NEXT;			//get the next packet
							else{											//if an error
								OutputReply[4] = res;				//send the error
								ScreenState = STATE_NOT_STARTED;
								IsModel = false;
							}
						}
						ProgramTimeout = 30;							//allow message to be sent
						if (IsLocal == ADDR_IS_LOCAL)
							SendCount = 5;
					}
					writeptrA = 0;
				}
				else{
					writeptrA = 0;
					ScreenState = STATE_NOT_STARTED;
				}
				if (SendCount){
					write(serial_fd, OutputReply, SendCount);
					SendCount = 0;
				}
			}
		}
		_time_delay(10);
		if (ProgramTimeout){
			ProgramTimeout--;
			if (ProgramTimeout == 0){
				ScreenState = STATE_NOT_STARTED;
				IsModel = false;
				writeptrA = 0;
			}
		}
	}
}

static void Serial_RX_ISR( pointer user_isr_ptr ){
	int_32 c;
	uint_8 s;
	UART_MemMapPtr sci_ptr = UART1_BASE_PTR;

	s = sci_ptr->S1;								//read again after every byte read in
	while(1){									//start receiving
		if (s & RECEIVE_BUFFER_OVERFLOW){		//if the buffer overflowed
			c = sci_ptr->D;								//reading the D register clears the overflow. invalid data so discard
			sci_ptr->CFIFO = FLUSH_RCV_BUFFER;			//clear the receive buffer
			break;										//we are done
		}
		else if (s & RECEIVE_BUFFER_FULL){				//if at least 1 byte
			c = sci_ptr->D;								//read it in
			InputMessage422[writeptrA++] = c;					//save it to our receive buffer
			if (writeptrA == MAX_INPUT_LENGTH)
				writeptrA = 0;
		}
		else{											//if nothing to read
			sci_ptr->CFIFO = FLUSH_RCV_BUFFER;			//just to be sure...
			break;										//we are done
		}
		s = sci_ptr->S1;								//read again after every byte read in
	}
}

uint_32 SendToFlash(uint_16 count, uint_8* data){
	uint_16 csum = 0, tmpcsum, a, b;
	uint_32 error = 0;

	for (a = 0; a < count - 2; a++)
		csum += *(data + a);
	tmpcsum = (*(data + a) << 8) + *(data + a + 1);
	if (csum != tmpcsum)
		return SREC_ERR_CHECKSUM;
	count -= 2;
	while (count > 0){
		if (count > 8)
			a = 8;
		else
			a = count;
		if ((LastAddress + a) >= END_FLEXRAM)
			return SREC_ERR_ERROR;
		if (write(flash_file, data, a) == IO_ERROR){
			return SREC_ERR_WRITE;
		}
		for (b = 0; b < a; b++){
			if (*(LastAddress + b) != *(data + b))
				return SREC_ERR_WRITE;
		}
		SRECLength += a;
		LastAddress += a;
		data += a;
		count -= a;
	}
	return SREC_OK;
}

