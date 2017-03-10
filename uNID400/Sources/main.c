#include <mqx.h>
#include <bsp.h>
#include <tfs.h>
#include <string.h>
#include "config.h"
#include "ram.h"


void MainTask(uint32_t value);
static void CopyCode();
static void CopyCode_end(void);
void CopyNewCode();

extern void SerialTask(uint32_t value);

#ifdef USE_BKPLN_TASK
extern void BackplaneTask(uint32_t value);
#else
extern void BackplaneInit();
#endif

extern void RAMInit();
extern void PeriphInit();
extern void ReadSFPModule(uint_8 sfp);
extern bool ChangeTXDisable(uint_8 sfp, uint_8 disable);
extern bool ChangeRXRate(uint_8 sfp);
extern bool ChangeTXRate(uint_8 sfp);
extern bool ChangePower(uint_8 sfp);
extern void DefaultSFPData(uint_8 side);
extern void DefaultSFPDataA2(uint_8 side);
extern void DefaultSFPSettings();
extern void DefaultPM();


#ifdef HAS_EXT_RTC
//conversion routines and data for event log
void LogEvent(uint_8 event, uint_8 source);
void RTC_GetTime(LDD_RTC_TTime *TimePtr);
void RTC_SetTime(LDD_RTC_TTime *TimePtr);
void RTC_SetDate(LDD_RTC_TTime *TimePtr);
void SecondsToTime(uint_32 Seconds, LDD_RTC_TTime *TimePtr);
bool TimeToSeconds(LDD_RTC_TTime *TimePtr, uint_32 *Seconds);

/* Table of month length (in days) */
const uint8_t ULY[] = {0U,31U,28U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U}; /* Non-leap-year */
const uint8_t  LY[] = {0U,31U,29U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U}; /* Leap-year */
/* Number of days from begin of the year */
const uint16_t MONTH_DAYS[] = {0U,0U,31U,59U,90U,120U,151U,181U,212U,243U,273U,304U,334U}; /* Leap-year */
#endif

/* Task IDs */
typedef enum{
	MAIN_TASK = 1,
	SERIAL_TASK,
#ifdef USE_BKPLN_TASK
	BKPLN_TASK,
#endif
	NO_TASK
}tasks;


const TASK_TEMPLATE_STRUCT  MQX_template_list[] =
{
   /* Task Index,   Function,      Stack,  Priority, Name,     Attributes,          Param, Time Slice */
    { MAIN_TASK,    MainTask,      2500,   8,        "main",   MQX_AUTO_START_TASK, 0,     0 },
	{ SERIAL_TASK,  SerialTask,    1000,   9,        "serial", MQX_TIME_SLICE_TASK, 0,     0 },
#ifdef USE_BKPLN_TASK
	{ BKPLN_TASK,   BackplaneTask, 1000,   9,        "bkpln",  MQX_TIME_SLICE_TASK, 0,     0 },
#endif
    { 0 }
};

/*TASK*-----------------------------------------------------
*
* Task Name    : world_task
* Comments     :
*    This task creates hello_task and then prints " World ".
*
*END*-----------------------------------------------------*/

void MainTask
   (
		   unsigned long value
   )
{
	uint_8 counts, a;
	WDOG_MemMapPtr wdPtr = WDOG_BASE_PTR;
	bool major;


	RAMInit();
	PeriphInit();

	DefaultSFPData(SOURCE_NET_P_SFP);
	DefaultSFPData(SOURCE_NET_S_SFP);
	DefaultSFPData(SOURCE_CUST_P_SFP);
	DefaultSFPData(SOURCE_CUST_S_SFP);
	DefaultSFPDataA2(SOURCE_NET_P_SFP);
	DefaultSFPDataA2(SOURCE_NET_S_SFP);
	DefaultSFPDataA2(SOURCE_CUST_P_SFP);
	DefaultSFPDataA2(SOURCE_CUST_S_SFP);

	if (_lwsem_create(&TIMER_SEM, 0) != MQX_OK) {		//this gives us a 1 sec tick from the RTC for the main loop
		_task_block();
	}   
	if (_task_create(0, SERIAL_TASK, 0) == MQX_NULL_TASK_ID)
		_task_block();
#ifdef USE_BKPLN_TASK
	if (_task_create(0, BKPLN_TASK, 0) == MQX_NULL_TASK_ID)
		_task_block();
#else
	BackplaneInit();
#endif
	LEDState[PWR_RST] = LED_OFF;

	_int_disable();
	wdPtr->UNLOCK = 0xc520;
	wdPtr->UNLOCK = 0xd928;
	wdPtr->TOVALH = 0x02ff;
	wdPtr->TOVALL = 0xffff;
	wdPtr->STCTRLH = 0x01d3; //enable WD
	_int_enable();
//get initial values
	a = PortD_GetFieldValue(PortD_DeviceData, ModulePresent);
	NoSFP[SOURCE_NET_P_SFP] = a & NET_P_PIN;
	NoSFP[SOURCE_NET_S_SFP] = a & NET_S_PIN;
	NoSFP[SOURCE_CUST_P_SFP] = a & CUST_P_PIN;
	NoSFP[SOURCE_CUST_S_SFP] = a & CUST_S_PIN;
	counts = 0;
	while(1){
#ifdef SHOW_HEARTBEAT

		if (counts & 0x1)
			Clear_LED_ClrVal(NULL);
		else
			Clear_LED_SetVal(NULL);
#endif
		counts++;
		if (DefaultMiscSettings){
			DefaultMiscSettings = false;
			DefaultSFPSettings();
#if HAS_INTERNAL_ELOG || HAS_MEMO_PAD
			DefaultPM();
#endif
		}
		a = PortE_GetFieldValue(PortE_DeviceData, Switches);
		if (a != SwitchOptions){
			SwitchOptions = a;
		}

#ifdef HAS_CRAFT_FW_UPDATE
		if (RestartUART){
			RestartUART = false;
			_task_abort(_task_get_id_from_name("craft"));
		}
		if (Reboot){					//if code updated
			CopyNewCode();
		}
#endif
		if (ReadSFPS){										//time to read the SFPs
			ReadSFPS = 0;
			ReadSFPModule(SOURCE_NET_P_SFP);
			ReadSFPModule(SOURCE_NET_S_SFP);
			ReadSFPModule(SOURCE_CUST_P_SFP);
			ReadSFPModule(SOURCE_CUST_S_SFP);
		}
/*
		if (!EventMismatch && NetSFPType > SFP_NONE && CustSFPType > SFP_NONE && NetSFPType != CustSFPType){
			LogEvent(EVENT_SFP_MISMATCH, SOURCE_NET_CUST_SFP);
			EventMismatch = true;
		}
		else if (EventMismatch && NetSFPType == CustSFPType){
			LogEvent(EVENT_SFP_MISMATCH_CLEAR, SOURCE_NET_CUST_SFP);
			EventMismatch = false;
		}
*/
		//below are for provisioning
		for (a = 0; a < 4; a++){
			if (tmpflags[a] & CHANGE_DISABLE){
				if (a == SOURCE_NET_P_SFP || a == SOURCE_NET_S_SFP){
					if (ChangeTXDisable(a, TmpNetTXDisableState))
						tmpflags[a] &= ~CHANGE_DISABLE;
				}
				else{
					if (ChangeTXDisable(a, TmpCustTXDisableState))
						tmpflags[a] &= ~CHANGE_DISABLE;
				}
			}
			if (tmpflags[a] & CHANGE_RX_RATE){
				if (ChangeRXRate(a))
					tmpflags[a] &= ~CHANGE_RX_RATE;
			}
			if (tmpflags[a] & CHANGE_TX_RATE){
				if (ChangeTXRate(a))
					tmpflags[a] &= ~CHANGE_TX_RATE;
			}
			if (tmpflags[a] & CHANGE_POWER){
				if (ChangePower(a))
					tmpflags[a] &= ~CHANGE_POWER;
			}
		}
		//check for SFP major, minor alarms...
		if (SFPStatus[SOURCE_NET_P_SFP].EventAlarmFlags != 0 || (SFPStatus[SOURCE_NET_P_SFP].SFPFlags & SFP_EVENT_TX_FAULT))
			LEDState[NET_P_PROB] = LED_ON;
		else if (SFPStatus[SOURCE_NET_P_SFP].EventWarnFlags != 0)
			LEDState[NET_P_PROB] = LED_BLINK;
		else
			LEDState[NET_P_PROB] = LED_OFF;

		if (SFPStatus[SOURCE_NET_S_SFP].EventAlarmFlags != 0 || (SFPStatus[SOURCE_NET_S_SFP].SFPFlags & SFP_EVENT_TX_FAULT))
			LEDState[NET_S_PROB] = LED_ON;
		else if (SFPStatus[SOURCE_NET_S_SFP].EventWarnFlags != 0)
			LEDState[NET_S_PROB] = LED_BLINK;
		else
			LEDState[NET_S_PROB] = LED_OFF;

		if (SFPStatus[SOURCE_CUST_P_SFP].EventAlarmFlags != 0 || (SFPStatus[SOURCE_CUST_P_SFP].SFPFlags & SFP_EVENT_TX_FAULT))
			LEDState[CUST_P_PROB] = LED_ON;
		else if (SFPStatus[SOURCE_CUST_P_SFP].EventWarnFlags != 0)
			LEDState[CUST_P_PROB] = LED_BLINK;
		else
			LEDState[CUST_P_PROB] = LED_OFF;

		if (SFPStatus[SOURCE_CUST_S_SFP].EventAlarmFlags != 0 || (SFPStatus[SOURCE_CUST_S_SFP].SFPFlags & SFP_EVENT_TX_FAULT))
			LEDState[CUST_S_PROB] = LED_ON;
		else if (SFPStatus[SOURCE_CUST_S_SFP].EventWarnFlags != 0)
			LEDState[CUST_S_PROB] = LED_BLINK;
		else
			LEDState[CUST_S_PROB] = LED_OFF;

		//check for UNID major, minor alarms...
		major = false;
		if ((NoSFP[SOURCE_NET_P_SFP] && NoSFP[SOURCE_NET_S_SFP]) || (NoSFP[SOURCE_CUST_P_SFP] && NoSFP[SOURCE_CUST_S_SFP]) || EventMismatch)//no sfp or sfp mismatch
			major = true;
/*
//		NOT SURE WHAT TO DO HERE YET...
		if (LEDState[CUST_P_SPEED] != LEDState[NET_P_SPEED]){	//or different speed for fiber
			if (!EventNetSpeedMismatch)
				LogEvent(EVENT_SFP_SPEED_BAD, SOURCE_NET_CUST_SFP);
			EventNetSpeedMismatch = true;
			major = true;
		}
		else if (EventNetSpeedMismatch){
			LogEvent(EVENT_SFP_SPEED_GOOD, SOURCE_NET_CUST_SFP);
			EventNetSpeedMismatch = false;
		}
		if (major){
			LEDState[NID_MAJOR] = LED_ON;
			LEDState[NID_MINOR] = LED_OFF;
		}
		else{
			LEDState[NID_MAJOR] = LED_OFF;
			if ((!NoSFPN && (!EventNetSFPCommOk || (NetDDMImplemented && !EventNetA2OK))) ||	//major takes precedence over minor...
				(!NoSFPC && (!EventCustSFPCommOk || (CustDDMImplemented && !EventCustA2OK)))){
				LEDState[NID_MINOR] = LED_ON;			//comm lost to SFP
			}
			else
				LEDState[NID_MINOR] = LED_OFF;
		}
*/
		_lwsem_wait(&TIMER_SEM);			//wait until 1 second timeout
		_int_disable();
		wdPtr->REFRESH = 0xa602;
		wdPtr->REFRESH = 0xb480;
		_int_enable();
	}
}

void LogEvent(uint8_t event, uint8_t source){
	struct eventlog *ePtr;

#ifdef HAS_INTERNAL_ELOG
	if (!PowerDownTime)		//when getting power down time, we read different registers. EventTime has the PD time already
		RTC_GetTime(&EventTime);
	ePtr = EventLog;
	ePtr += LastEvent;		//point to the flash
	TimeToSeconds(&EventTime, &WorkingLog.TimeStamp);	//convert it to uint32
	WorkingLog.EventType = event;
	WorkingLog.Source = source;
	fseek(flash_file, (int_32)ePtr, IO_SEEK_SET);		//got to the flash
	write(flash_file, (uint_8*)&WorkingLog.TimeStamp, 8);	//and write it
	LastEvent++;											//go to the next location
	if (LastEvent == TOTAL_EVENTS)							//roll over
		LastEvent = 0;
	if (LastEvent == 0 || LastEvent == 512){				//if the next one is in the other sector
		ePtr = EventLog;
		ePtr += LastEvent;
    	fseek(flash_file, (int_32)ePtr, IO_SEEK_SET);
		_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL); 		//erase it first
	}
	if (NumEvents == MAX_EVENTS){
		FirstEvent++;
		if (FirstEvent == TOTAL_EVENTS)
			FirstEvent = 0;
	}
	else
		NumEvents++;
#else
	WorkingLog[LastEvent].EventType = event;
	WorkingLog[LastEvent++].Source = source;
	if (LastEvent == MAX_EVENTS)
		LastEvent = 0;
	NumEvents++;
#endif
}

#ifdef HAS_EXT_RTC
void RTC_GetTime(LDD_RTC_TTime *TimePtr){
	uint_8 rcvbuf[7], xmtbuf[2];

	ExtRTC_SPI_ReceiveBlock(RTCSPIPtr, &rcvbuf, 7);
	xmtbuf[0] = RTC_READ;
	xmtbuf[1] = RTC_SECONDS;
	RTCSPIReceivedFlag = false;
	ExtRTC_SPI_SendBlock(RTCSPIPtr, &xmtbuf, 2);
	while (!RTCSPIReceivedFlag){
	}
	rcvbuf[0] &= 0x7f;
	TimePtr->Second = ((rcvbuf[0] >> 4) * 10) + (rcvbuf[0] & 0xf);
	TimePtr->Minute = ((rcvbuf[1] >> 4) * 10) + (rcvbuf[1] & 0xf);
	rcvbuf[2] &= 0x3f;
	TimePtr->Hour = ((rcvbuf[2] >> 4) * 10) + (rcvbuf[2] & 0xf);
	TimePtr->DayOfWeek = rcvbuf[3] & 0x7;
	TimePtr->Day = ((rcvbuf[4] >> 4) * 10) + (rcvbuf[4] & 0xf);
	rcvbuf[5] &= 0x1f;
	TimePtr->Month = ((rcvbuf[5] >> 4) * 10) + (rcvbuf[5] & 0xf);
	TimePtr->Year = (((rcvbuf[6] >> 4) * 10) + (rcvbuf[6] & 0xf)) + 2000;
}

void RTC_SetTime(LDD_RTC_TTime *TimePtr){
	uint_8 xmtbuf[6];

	xmtbuf[0] = RTC_WRITE;
	xmtbuf[1] = RTC_SECONDS_FRACTION;
	xmtbuf[2] = 0;
	xmtbuf[3] = 0x80 | ((TimePtr->Second / 10) << 4) | (TimePtr->Second % 10);
	xmtbuf[4] = ((TimePtr->Minute / 10) << 4) | (TimePtr->Minute % 10);
	xmtbuf[5] = ((TimePtr->Hour / 10) << 4) | (TimePtr->Hour % 10);
	RTCSPIReceivedFlag = false;
	ExtRTC_SPI_SendBlock(RTCSPIPtr, &xmtbuf, 6);
	while (!RTCSPIReceivedFlag){
	}
}

void RTC_SetDate(LDD_RTC_TTime *TimePtr){
	uint_8 xmtbuf[5];

	xmtbuf[0] = RTC_WRITE;
	xmtbuf[1] = RTC_DATE;
	xmtbuf[2] = ((TimePtr->Day / 10) << 4) | (TimePtr->Day % 10);
	xmtbuf[3] = ((TimePtr->Month / 10) << 4) | (TimePtr->Month % 10);
	xmtbuf[4] = (((TimePtr->Year - 2000) / 10) << 4) | ((TimePtr->Year - 2000) % 10);
	RTCSPIReceivedFlag = false;
	ExtRTC_SPI_SendBlock(RTCSPIPtr, &xmtbuf, 5);
	while (!RTCSPIReceivedFlag){
	}
}

//These two routines are taken from RTC.c and modified to just convert to / from uint32 for our timestamp conversions
void SecondsToTime(uint_32 Seconds, LDD_RTC_TTime *TimePtr){
	uint_32 Days, x;

	Seconds--;
	Days = Seconds / 86400U;             /* Days */
	Seconds = Seconds % 86400U;          /* Seconds left */
	TimePtr->Hour = Seconds / 3600U;     /* Hours */
	Seconds = Seconds % 3600u;           /* Seconds left */
	TimePtr->Minute = Seconds / 60U;     /* Minutes */
	TimePtr->Second = Seconds % 60U;     /* Seconds */
	TimePtr->DayOfWeek = (Days + 6U) % 7U; /* Day of week */
	TimePtr->Year = (4U * (Days / ((4U * 365U) + 1U))) + 2000U; /* Year */
	Days = Days % ((4U * 365U) + 1U);
	if (Days == ((0U * 365U) + 59U)) { /* 59 */
		TimePtr->Day = 29U;
	    TimePtr->Month = 2U;
	    return;
	} else if (Days > ((0U * 365U) + 59U)) {
		Days--;
	} else {
	}
	x =  Days / 365U;
	TimePtr->Year += x;
	Days -= x * 365U;
	for (x=1U; x <= 12U; x++) {
		if (Days < ULY[x]) {
			TimePtr->Month = x;
			break;
		} else {
			Days -= ULY[x];
		}
	}
	TimePtr->Day = Days + 1U;
}

bool TimeToSeconds(LDD_RTC_TTime *TimePtr, uint_32 *Seconds){
	if ((TimePtr->Year < 2000U) || (TimePtr->Year > 2099U) || (TimePtr->Month > 12U) || (TimePtr->Month == 0U) || (TimePtr->Day > 31U) || (TimePtr->Day == 0U)) { /* Test correctness of given parameters */
		return 0;                  /* If not correct then error */
	}
	if (TimePtr->Year & 3U) {            /* Is given year non-leap-one? */
		if (ULY[TimePtr->Month] < TimePtr->Day) { /* Does the obtained number of days exceed number of days in the appropriate month & year? */
			return 0;                /* If yes (incorrect date inserted) then error */
		}
	} else {                             /* Is given year leap-one? */
		if (LY[TimePtr->Month] < TimePtr->Day) { /* Does the obtained number of days exceed number of days in the appropriate month & year? */
			return 0;                /* If yes (incorrect date inserted) then error */
		}
	}
	*Seconds = ((TimePtr->Year - 2000U) * 365U) + (((TimePtr->Year - 2000U) + 3U) / 4U); /* Compute number of days from 2000 till given year */
	*Seconds += MONTH_DAYS[TimePtr->Month]; /* Add number of days till given month */
	*Seconds += TimePtr->Day;             /* Add days in given month */
	if ((TimePtr->Year & 3U) || (TimePtr->Month <= 2U)) { /* For non-leap year or month <= 2, decrement day counter */
		*Seconds -= 1;
	}
	*Seconds = (*Seconds * 86400U) + (TimePtr->Hour * 3600U) + (TimePtr->Minute * 60U) + TimePtr->Second;
	*Seconds += 1;
	return 1;
}
#endif

#ifdef HAS_ETHERNET

void DoTFTPUpdate(){
	uint_32 error;
	WDOG_MemMapPtr wdPtr = WDOG_BASE_PTR;

	TFTPUpdateState = 2;
	_int_disable();
	wdPtr->UNLOCK = 0xc520;
	wdPtr->UNLOCK = 0xd928;
	wdPtr->STCTRLH = 0x01d2; //disable WD
    _int_enable();
	TFTPError = 0;
	if (CheckOnSubnet(TFTPAdress)){
		ARP_request(ihandle, ip_data.ip, TFTPAdress);		//arp the tftp address
		_time_delay(2000);
		if (!ARP_is_complete(ihandle, TFTPAdress))			//see if it replied to the arp
			TFTPError = 3;									//if not then done
	}
	if (TFTPError == 0){
		for (error = 0; error < 0x6B000; error += 0x1000){			//erase bank 1 flash area
			fseek(flash_file, error, IO_SEEK_SET);
			if (_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL) != MQX_OK)
				break;
		}
		if (error == 0x6B000){
			TFTPError = RTCS_load_TFTP_SREC(TFTPAdress, FileName, flash_file, &SRECLength);		//get new file
			if (!TFTPError){
				if (strcmp(RcvdProductString, ProductString)){
					TFTPError = RTCSERR_SREC_NOTUB;
				}
			}
		}
		else
			TFTPError = 5;
	}
	CopyNewCode();
}
#endif

//firmware transfer is done, now copy the code to ram, log the event, kill tasks, run the copy code function, and reset when done
void CopyNewCode(){
	void (* RunInRAM)();
	uint_8* ram_code_ptr;
	_mem_size CopyCode_function_start;
	WDOG_MemMapPtr wdPtr = WDOG_BASE_PTR;

	SRECLength |= 0x10000000;		//offset to bank 1
	if (!TFTPError){			//if no errors, update the code
		CopyCode_function_start = (_mem_size)CopyCode & ~1;  // Remove thumb2 flag from the address
		ram_code_ptr = _mem_alloc((uint_8*)CopyCode_end  - (uint_8*)CopyCode_function_start); // allocate space to run flash command out of RAM
		if (ram_code_ptr)
			_mem_copy ((uint_8*)CopyCode_function_start, ram_code_ptr, (uint_8*)CopyCode_end  - (uint_8*)CopyCode_function_start); // copy code to RAM buffer
		else{
			TFTPError = 6;
			return;
		}
	}
	TFTPUpdateState = 3;
	if (!TFTPError){			//if no errors, update the code
		LogEvent(EVENT_FW_UPDATE, SOURCE_VLX_MODULE);
		_time_delay(5000);		//allow time for message to be sent to user
		_task_stop_preemption();
		RunInRAM = (void(*)())((uint_8*)ram_code_ptr + 1);
		__disable_interrupt ();
		RunInRAM();				//erase bank 0, copy the code from bank 1 to bank 0, and do a reset
		__enable_interrupt ();	//should NEVER hit this, but maybe the flash did not erase.
	}   	   	   	   	   	   	   //we could still be screwed if even one sector erased, but...
#ifdef HAS_ETHERNET				//we dont disable for craft...
	_int_disable();
	wdPtr->UNLOCK = 0xc520;
	wdPtr->UNLOCK = 0xd928;
	wdPtr->STCTRLH = 0x01d3; //enable WD
    _int_enable();
#endif
}

//when TFTP is complete we need to copy the code from the upper block to the code space.
//This obviously can not run out of code space. This routine is copied to RAM and is run from RAM.
static void CopyCode(){
	uint_32 addr;
    FTFL_MemMapPtr ftfl_ptr = FTFL_BASE_PTR;
    uint_32 data, data1;
    uint_8 error = 0;

    while (0 == (ftfl_ptr->FSTAT & FTFL_FSTAT_CCIF_MASK))
    {}
    if (ftfl_ptr->FSTAT & FTFL_FSTAT_RDCOLERR_MASK)
        ftfl_ptr->FSTAT |= FTFL_FSTAT_RDCOLERR_MASK;
    if (ftfl_ptr->FSTAT & FTFL_FSTAT_ACCERR_MASK)
        ftfl_ptr->FSTAT |= FTFL_FSTAT_ACCERR_MASK;
    if (ftfl_ptr->FSTAT & FTFL_FSTAT_FPVIOL_MASK)
        ftfl_ptr->FSTAT |= FTFL_FSTAT_FPVIOL_MASK;
    for (addr = 0; addr < 0x80000; addr += 0x40000){
    	ftfl_ptr->FCCOB3 = addr;
    	ftfl_ptr->FCCOB2 = addr >> 8;
    	ftfl_ptr->FCCOB1 = addr >> 16;
    	ftfl_ptr->FCCOB0 = 0x08;					//erase blocks
    	ftfl_ptr->FSTAT |= FTFL_FSTAT_CCIF_MASK;
    	while (0 == (ftfl_ptr->FSTAT & FTFL_FSTAT_CCIF_MASK))	// wait until execution complete
    	{}
    	if ((ftfl_ptr->FSTAT & 0x70) != 0){		//if this happens the flash did not erase
    		error++;
    		if (error == 3)		//try 3 times
    			while(1){}	// we are screwed
    	    if (ftfl_ptr->FSTAT & FTFL_FSTAT_RDCOLERR_MASK)
    	        ftfl_ptr->FSTAT |= FTFL_FSTAT_RDCOLERR_MASK;
    	    if (ftfl_ptr->FSTAT & FTFL_FSTAT_ACCERR_MASK)
    	        ftfl_ptr->FSTAT |= FTFL_FSTAT_ACCERR_MASK;
    	    if (ftfl_ptr->FSTAT & FTFL_FSTAT_FPVIOL_MASK)
    	        ftfl_ptr->FSTAT |= FTFL_FSTAT_FPVIOL_MASK;
    	    addr -= 0x40000;			//try again
    	}
    	else
    		error = 0;
    }
    for (addr = 0x10000000; addr <= SRECLength; addr += 4){
        ftfl_ptr->FCCOB3 = addr;
        ftfl_ptr->FCCOB2 = addr >> 8;
        ftfl_ptr->FCCOB1 = addr >> 16;
    	data = *(uint_32*)addr;
        ftfl_ptr->FCCOB7 = data;
        ftfl_ptr->FCCOB6 = data >> 8;
        ftfl_ptr->FCCOB5 = data >> 16;
        ftfl_ptr->FCCOB4 = data >> 24;
        addr += 4;
    	data1 = *(uint_32*)addr;
        ftfl_ptr->FCCOBB = data1;
        ftfl_ptr->FCCOBA = data1 >> 8;
        ftfl_ptr->FCCOB9 = data1 >> 16;
        ftfl_ptr->FCCOB8 = data1 >> 24;
        ftfl_ptr->FCCOB0 = 0x07;					//program 8 bytes at a time
        ftfl_ptr->FSTAT |= FTFL_FSTAT_CCIF_MASK;
        while (0 == (ftfl_ptr->FSTAT & FTFL_FSTAT_CCIF_MASK))  // wait until execution complete
        {}
        if ((ftfl_ptr->FSTAT & 0x70) != 0){		//dont need this because we erased the flash
    		error++;
    		if (error == 3)		//try 3 times
    			while(1){}	// we are screwed
    	    if (ftfl_ptr->FSTAT & FTFL_FSTAT_RDCOLERR_MASK)
    	        ftfl_ptr->FSTAT |= FTFL_FSTAT_RDCOLERR_MASK;
    	    if (ftfl_ptr->FSTAT & FTFL_FSTAT_ACCERR_MASK)
    	        ftfl_ptr->FSTAT |= FTFL_FSTAT_ACCERR_MASK;
    	    if (ftfl_ptr->FSTAT & FTFL_FSTAT_FPVIOL_MASK)
    	        ftfl_ptr->FSTAT |= FTFL_FSTAT_FPVIOL_MASK;
    	    addr -= 8;			//try again
        }
        else{
        	error = 0;
        	if (data != *(uint_32*)(addr - 4) || data1 != *(uint_32*)addr){
    			while(1){}	// we are screwed
        	}
        }
    }
	SCB_AIRCR = SCB_AIRCR_VECTKEY(0x5FA)| SCB_AIRCR_SYSRESETREQ_MASK; //reset micro
	while(1);	// wait for reset to occur
}
static void CopyCode_end(void)			//just to mark the end of the copycode function
{}

