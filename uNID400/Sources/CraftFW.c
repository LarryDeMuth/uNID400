/*
 * CraftFW.c
 *
 *  Created on: Apr 10, 2016
 *      Author: Larry
 */
#include "config.h"
#ifdef HAS_CRAFT_FW_UPDATE

#include <mqx.h>
#include "extern.h"
#include "craftscreens.h"
#include <string.h>

extern uint_32 SREC_start  (pointer ff_data, uint_32* size);
extern uint_32 SREC_decode (uint_32 size, uchar_ptr data);
extern uint_32 SREC_eod    (void);

extern uint_8 BufferA[];
extern uint_16 writeptrA;
extern char ScreenString[];
extern uint_8 IncomingByteNumber;
extern MQX_FILE_PTR serial_fd;

//the FW update is taken from the 12K expansion, so this may seem weird, but it makes coding consistent
#define PROGRAM_CODE	9

#define PROGRAM_START		0xAA;

enum{
	NOT_STARTED = 0,
	START_PROGRAM,
	SEND_NEXT,
	PROGRAM_COMPLETE,
	PROGRAM_ERROR
};

// possible errors
/*
#define RTCSERR_SREC_CHAR           0x80
#define RTCSERR_SREC_RECORD         0x81
#define RTCSERR_SREC_SHORT          0x82
#define RTCSERR_SREC_CHECKSUM       0x83
#define RTCSERR_SREC_START          0x84
#define RTCSERR_SREC_END            0x85
#define RTCSERR_SREC_EOF            0x86
#define RTCSERR_SREC_ERROR          0x87
#define RTCSERR_SREC_NOTUB          0x88
*/

void FWUpdateTask(){
	uint_32 error;
	int_32 res;

	ProgramTimeout = 1000;
	while (1){
		if (writeptrA > 1){
			if ((BufferA[1] + 2) == writeptrA){
			if (BufferA[0] == PROGRAM_CODE){
				IncomingByteNumber = 0;
				if (ScreenState == NOT_STARTED){
					writeptrA = 0;
					if (BufferA[1] == 0){						//no other bytes with the start command
						ProgramTimeout = 1000;							//set our timeout
						for (error = 0; error < 0x6B000; error += 0x1000){			//erase bank 1 flash area
							fseek(flash_file, error, IO_SEEK_SET);
							if (_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL) != MQX_OK)
								break;
						}
						SRECLength = 0;
						TFTPError = 0;
						if (error == 0x6B000 && !SREC_start(flash_file, &SRECLength)){			//set up tftp code
							ScreenState = START_PROGRAM;					//go to start program state
							ScreenString[0] = PROGRAM_CODE;
							ScreenString[1] = 1;
							ScreenString[2] = START_PROGRAM;			//tell other end we are ready before switching
							ProgramTimeout = 300;							//set our timeout
						}
						else{												//failure
							ScreenString[0] = PROGRAM_CODE;
							ScreenString[1] = 1;
							ScreenString[2] = PROGRAM_ERROR;
							ProgramTimeout = 30;							//let message be sent then let it expire
							TFTPError = 0x100;
						}
						IncomingByteNumber = 3;
					}
				}
				else if (ScreenState == START_PROGRAM){
					if (BufferA[1] == 0){					//no bytes = done
						if (SRECLength == 0){							//if no data received
							ScreenString[0] = PROGRAM_CODE;
							ScreenString[1] = 1;
							ScreenString[2] = PROGRAM_ERROR;
						}
						else{
							ScreenString[0] = PROGRAM_CODE;
							ScreenString[1] = 1;
							res = SREC_eod();
							if (!res){									//if no end  error
								if (strcmp(RcvdProductString, ProductString)){
									ScreenString[2] = RTCSERR_SREC_NOTUB & 0xff;	//wrong product
								}
								else{
									ScreenString[2] = PROGRAM_COMPLETE;	//OK we got it so reboot
									Reboot = 1;
								}
							}
							else{
								ScreenString[2] = res & 0xff;				//send the error
								TFTPError = res;
							}
						}
						ProgramTimeout = 3;							//allow message to be sent
						IncomingByteNumber = 3;
					}
					else{												//if data...
						res = SREC_decode(BufferA[1], (uint_8*)&BufferA[2]);	//verify it
						if (!res){										//if no errors...
							ProgramTimeout = 300;						//dont let timeout expire
							ScreenString[0] = PROGRAM_CODE;
							ScreenString[1] = 1;
							ScreenString[2] = SEND_NEXT;			//get the next packet
						}
						else{											//if an error
							ScreenString[0] = PROGRAM_CODE;
							ScreenString[1] = 1;
							ScreenString[2] = res & 0xff;				//send the error
							TFTPError = res;
							ScreenState = NOT_STARTED;
							ProgramTimeout = 3;							//allow message to be sent
						}
						IncomingByteNumber = 3;
					}
					writeptrA = 0;
				}
				if (IncomingByteNumber > 0)
					write(serial_fd, ScreenString, IncomingByteNumber);
			}
			}
		}
		_time_delay(10);
	}
}

#endif
