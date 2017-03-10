/*
 * Init.c
 *
 *  Created on: Nov 25, 2014
 *      Author: Larry
 */
#include <mqx.h>
#include <string.h>
#include "extern.h"

const char Pword[] = "admin";

void RAMInit();
void PeriphInit();
void DefaultSFPSettings();
void DefaultPM();

extern void LogEvent(uint_8 event, uint_8 source);
extern void RTC_GetTime(LDD_RTC_TTime *TimePtr);

const uint_8 DefaultRTCData[11] = {RTC_WRITE, RTC_SECONDS_FRACTION, 0x00, 0x80, 0x00, 0x00, 0x09, 0x01, 0x01, 0x00, 0x00};

void RAMInit(){
	uint_16 a;
	uint_32 val;
	struct memolog *mPtr;
	struct eventlog *ePtr;
	FLEXNVM_PROG_PART_STRUCT part_param;
    /* Open the flash device */
	
    flash_file = fopen("flashx:bank1", NULL);
    if (NULL == flash_file) {
        _task_block();
    }
    part_param.EE_DATA_SIZE_CODE = 0;
    part_param.FLEXNVM_PART_CODE = 0;
    if (_io_ioctl(flash_file, FLEXNVM_IOCTL_GET_PARTITION_CODE, &part_param) != IO_OK){
        _task_block();
    }	
// check FlexNVM partition settings (128 bytes for EE backup, 96k for data) if no partition, partition it
    if (FLEXNVM_PART_CODE_NOPART == part_param.FLEXNVM_PART_CODE) {
        part_param.EE_DATA_SIZE_CODE = (FLEXNVM_EE_SPLIT_1_1 | FLEXNVM_EE_SIZE_128);
        part_param.FLEXNVM_PART_CODE = FLEXNVM_PART_CODE_DATA96_EE32;
        _io_ioctl(flash_file, FLEXNVM_IOCTL_SET_PARTITION_CODE, &part_param);         
        // switch FlexRAM to EEPROM mode 
        val = FLEXNVM_FLEXRAM_EE;
        _io_ioctl(flash_file, FLEXNVM_IOCTL_SET_FLEXRAM_FN, &val); 
    }
//The following tests are divided up so one change to something doesn't require everything to be defaulted 
    if (*SettingsState != SETTINGS_TEST){
    	DefaultSFPSettings();
    	_int_disable();
     	*SettingsState = SETTINGS_TEST;
        _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL); 
        _int_enable();
        DefaultTime = TRUE;		//default the time in the RTC
    }
    else
        DefaultTime = FALSE;	//don't default the time in the RTC
#if HAS_INTERNAL_ELOG || HAS_MEMO_PAD
    if (*PMStatsState != PM_DATA_TEST){
    	DefaultPM();
    	_int_disable();
     	*PMStatsState = PM_DATA_TEST;
        _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL); 
        _int_enable();
    }
    else{			//if already configured, find first, last, and number of events and memos
#else
    	{
#endif
#ifdef HAS_INTERNAL_ELOG
        LastEvent = FirstEvent = 0;
        NumEvents = 0;
        ePtr = EventLog;
        val = 0;
        for (a = 0; a < TOTAL_EVENTS; a++){					//find first, last, and number of events
        	fseek(flash_file, (int_32)ePtr, IO_SEEK_SET);
        	read(flash_file, &WorkingLog.TimeStamp, 4);
        	if (WorkingLog.TimeStamp != 0xffffffff){		//a valid event will have a timestamp
        		NumEvents++;								//inc number valid events
        		if (val == 0)								//if we havent hit the first non valid event (no timestamp) yet
        			LastEvent = a + 1;						//mark this as potential last
        	}
        	else if (LastEvent != 0)						//if a non valid event (no timestamp), and we found valid ones already
        		val = 1;									//whatever lastevent was IS the last event entered
        	ePtr++;
        }
        if (LastEvent == TOTAL_EVENTS){						//if the last valid was the last location
        	FirstEvent = TOTAL_EVENTS - MAX_EVENTS;			//the first MUST be at this
        	LastEvent = 0;									//roll over lastevent
        	NumEvents = MAX_EVENTS;							//we MUST have this number of events
        }
        if (NumEvents > MAX_EVENTS){						//if more than this number of events, find the first one
        	FirstEvent = LastEvent - MAX_EVENTS;			//the first is this
        	if (FirstEvent < 0)								//if roll over
        		FirstEvent += TOTAL_EVENTS;					//add total to get the first one
        	NumEvents = MAX_EVENTS;							//reset to max
        }
#else
        FirstEvent = 0;
        LastEvent = 0;
        NumEvents = 0;
#endif
#ifdef HAS_MEMO_PAD
        mPtr = MemoPad;
        NumMemos = 0;
        LastMemo = FirstMemo = 0;
        val = 0;
        for (a = 0; a < TOTAL_MEMO; a++){					//find first, last, and number of memos
        	fseek(flash_file, (int_32)mPtr, IO_SEEK_SET);
        	read(flash_file, &WorkingMemo.TimeStamp, 4);
        	if (WorkingMemo.TimeStamp != 0xffffffff){		//a valid memo will have a timestamp
        		NumMemos++;									//inc number valid memos
        		if (val == 0)								//if we havent hit the first non valid memo (no timestamp) yet
        			LastMemo = a + 1;						//mark this as potential last
        	}
        	else if (LastMemo != 0)							//if a non valid memo (no timestamp), and we found valid ones already
        		val = 1;									//whatever lastmemo was IS the last memo entered
        	mPtr++;
        }
        if (LastMemo == TOTAL_MEMO){						//if the last valid was the last location
        	FirstMemo = TOTAL_MEMO - MAX_MEMO;				//the first MUST be at 32
        	LastMemo = 0;									//roll over lastmemo
        	NumMemos = MAX_MEMO;							//we MUST have 32 memos
        }
        if (NumMemos > MAX_MEMO){							//if more than 32 memos, find the first one
        	FirstMemo = LastMemo - MAX_MEMO;				//the first is lastmemo - 32
        	if (FirstMemo < 0)								//if roll over
        		FirstMemo += TOTAL_MEMO;					//add total to get the first one
        	NumMemos = MAX_MEMO;							//reset to max
        }
#endif
    }

//initialize RAM
    ReadSFPS = TRUE;
#ifdef HAS_CRAFT_FW_UPDATE
    UARTFWUpdate = RestartUART = Reboot = false;
    ProgramTimeout = 0;
#endif
    SFPError = FALSE;
    SelectedSFP = 0xff;	//none
    TenMilisecondCount = 0;
    HundredMilisecondCount = 0;
    OneSecondCount = 0;
//    MLBInTimer = MLBOutTimer = 0;
//    MLBPressedState = false;
    FlashLED = 0;
    PulseLED = 0;
    SFPRcvBusy = SFPXmitBusy = 0;
    MuxRcvBusy = MuxXmitBusy = 0;
    RTCSPIRcvBusy = RTCSPIXmtBusy = 0;
    TempSPIRcvBusy = TempSPIXmtBusy = 0;
    EventMismatch = FALSE;
    UpdateProvision = 0;
    DefaultMiscSettings = false;
    DefaultSettings = false;
	TFTPUpdateState = 0;
	UnitAddress = 0xff;
	SwitchOptions = 0xff;
	for (a = 0; a < 4; a++)
		tmpflags[a] = 0;
	Reboot = false;
}

void PeriphInit(){
	uint_8 xmtbuf[11], rcvbuf[4];
	LDD_I2C_TBusState BusState;
	uint_8 error = 0;

	MuxPtr = Mux_I2C_Init(NULL);
	SFPPtr = SFPs_I2C_Init(NULL);
	Timer1Ptr = TU1_Init(NULL);
#ifdef HAS_EXT_RTC
	RTCSPIPtr = ExtRTC_SPI_Init(NULL);
#endif
	UnitAddress = PortE_GetFieldValue(PortE_DeviceData, Address);
	SwitchOptions = PortE_GetFieldValue(PortE_DeviceData, Switches);
//INIT LED'S AND OUTPUT PINS
	LEDState[CUST_P_OP] = LED_OFF;
	LEDState[CUST_P_PROB] = LED_OFF;
	LEDState[CUST_P_SPEED] = LED_OFF;
	LEDState[CUST_P_LOS] = LED_ON;

	LEDState[CUST_S_OP] = LED_OFF;
	LEDState[CUST_S_PROB] = LED_OFF;
	LEDState[CUST_S_SPEED] = LED_OFF;
	LEDState[CUST_S_LOS] = LED_ON;

	LEDState[NET_P_OP] = LED_OFF;
	LEDState[NET_P_PROB] = LED_OFF;
	LEDState[NET_P_SPEED] = LED_OFF;
	LEDState[NET_P_LOS] = LED_ON;

	LEDState[NET_S_OP] = LED_OFF;
	LEDState[NET_S_PROB] = LED_OFF;
	LEDState[NET_S_SPEED] = LED_OFF;
	LEDState[NET_S_LOS] = LED_ON;

	LEDState[PWR_LED] = LED_OFF;
	LEDState[PWR_RST] = LED_BLINK;
	LEDState[PROT_LED] = LED_OFF;
	LEDState[MON_LED] = LED_OFF;
	LEDState[NID_MAJOR] = LED_OFF;
	LEDState[NID_MINOR] = LED_OFF;

	PortC_ClearFieldBits(PortC_DeviceData, NP_Pins, TX_DISABLE_PIN);
	PortC_ClearFieldBits(PortC_DeviceData, NS_Pins, TX_DISABLE_PIN);
	PortC_ClearFieldBits(PortC_DeviceData, CP_Pins, TX_DISABLE_PIN);
	PortC_ClearFieldBits(PortC_DeviceData, CS_Pins, TX_DISABLE_PIN);

	if (*NetRXRateState == LOW_RATE_RX){
		PortC_ClearFieldBits(PortC_DeviceData, NP_Pins, RATE_SELECT_PIN);
		PortC_ClearFieldBits(PortC_DeviceData, NS_Pins, RATE_SELECT_PIN);
	}
	else{
		PortC_SetFieldBits(PortC_DeviceData, NP_Pins, RATE_SELECT_PIN);
		PortC_SetFieldBits(PortC_DeviceData, NS_Pins, RATE_SELECT_PIN);
	}
	if (*CustRXRateState == LOW_RATE_RX){
		PortC_ClearFieldBits(PortC_DeviceData, CP_Pins, RATE_SELECT_PIN);
		PortC_ClearFieldBits(PortC_DeviceData, CS_Pins, RATE_SELECT_PIN);
	}
	else{
		PortC_SetFieldBits(PortC_DeviceData, CP_Pins, RATE_SELECT_PIN);
		PortC_SetFieldBits(PortC_DeviceData, CS_Pins, RATE_SELECT_PIN);
	}
//INIT MUX
	error = 0;
	do{
		Mux_I2C_CheckBus(MuxPtr, &BusState);				//wait for bus to go idle
		error++;										//but dont wait forever...
	}
	while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
	xmtbuf[0] = GLOBAL_INPUT_STATE;
	xmtbuf[1] = STATE_DEFAULT;
	Mux_I2C_MasterSendBlock(MuxPtr, xmtbuf, 2, LDD_I2C_SEND_STOP);	//write it out
	MuxDataTransmittedFlg = FALSE;
	while (!MuxDataTransmittedFlg){				//wait till write completes or timeout
	}
	error = 0;
	do{
		Mux_I2C_CheckBus(MuxPtr, &BusState);				//wait for bus to go idle
		error++;										//but dont wait forever...
	}
	while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
	xmtbuf[0] = GLOBAL_OUTPUT_LEVEL;
	xmtbuf[1] = LEVEL_DEFAULT;
	xmtbuf[2] = MODE_DEFAULT;
	Mux_I2C_MasterSendBlock(MuxPtr, xmtbuf, 3, LDD_I2C_SEND_STOP);	//write it out
	MuxDataTransmittedFlg = FALSE;
	while (!MuxDataTransmittedFlg){				//wait till write completes or timeout
	}
#ifdef HAS_EXT_RTC
//CHECK RTC
	rcvbuf[0] = 0xc0;
	ExtRTC_SPI_ReceiveBlock(SPIPtr, &rcvbuf, 1);
	xmtbuf[0] = RTC_READ;
	xmtbuf[1] = RTC_DAY;		//read day register. should have OSCON, VBAT, and VBATEN set.
	RTCSPIReceivedFlag = false;
	RTC_SPI_SendBlock(SPIPtr, &xmtbuf, 2);
	while (!RTCSPIReceivedFlag){
	}
	if (DefaultTime || (rcvbuf[0] & 0x38) != 0x38){	//if we are defaulting time or RTC wasnt configured
		PowerDownTime = false;
		RTCSPITransmittedFlag = false;
		ExtRTC_SPI_SendBlock(SPIPtr, &DefaultRTCData, sizeof(DefaultRTCData));
		while (!RTCSPITransmittedFlag){
		}
	    _time_delay(50);			//just some delay for RTC osc to start
	}
	else
		PowerDownTime = true;
    RTC_GetTime(&RTC_TTime);
	if (PowerDownTime){					//get the power loss time
		ExtRTC_SPI_ReceiveBlock(SPIPtr, &rcvbuf, 4);
		xmtbuf[0] = RTC_READ;
		xmtbuf[1] = RTC_PD_MINUTES;
		RTCSPIReceivedFlag = false;
		ExtRTC_SPI_SendBlock(SPIPtr, &xmtbuf, 2);
		while (!RTCSPIReceivedFlag){
		}
		EventTime.Second = 0;
		EventTime.Minute = ((rcvbuf[0] >> 4) * 10) + (rcvbuf[0] & 0xf);
		rcvbuf[1] &= 0x3f;
		EventTime.Hour = ((rcvbuf[1] >> 4) * 10) + (rcvbuf[1] & 0xf);
		EventTime.Day = ((rcvbuf[2] >> 4) * 10) + (rcvbuf[2] & 0xf);
		rcvbuf[3] &= 0x1f;
		EventTime.Month = ((rcvbuf[3] >> 4) * 10) + (rcvbuf[3] & 0xf);
		EventTime.Year = RTC_TTime.Year;
		if (EventTime.Month < RTC_TTime.Month)	//if month rollover
			EventTime.Year++;					//go to next year
		LogEvent(EVENT_POWERDOWN, SOURCE_VLX_MODULE);
		PowerDownTime = false;
	}
	LogEvent(EVENT_POWERUP, SOURCE_VLX_MODULE);
#endif
}

void DefaultSFPSettings(){
	_int_disable();
	*CircuitID = 0;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *NetTXDisableState = ENABLE_TX;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *CustTXDisableState = ENABLE_TX;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *NetTXRateState = HIGH_RATE_TX;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *CustTXRateState = HIGH_RATE_TX;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *NetTXPowerState = LOW_POWER;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *CustTXPowerState = LOW_POWER;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *NetRXRateState = HIGH_RATE_RX;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    *CustRXRateState = HIGH_RATE_RX;
    _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    _int_enable();
}

#if HAS_INTERNAL_ELOG || HAS_MEMO_PAD
void DefaultPM(){
	uint_32 val;

	_int_disable();
	for (val = 0x6B000; val < 0x70000; val += 0x1000){			//erase bank 1 flash area reserved for event log, and memo pad
		fseek(flash_file, val, IO_SEEK_SET);
		_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL);
	}
    _int_enable();
#ifdef HAS_INTERNAL_ELOG
    LastEvent = 1;
    FirstEvent = 0;
    NumEvents = 0;
#endif
#ifdef HAS_MEMO_PAD
    NumMemos = 0;
    LastMemo = 1;
    FirstMemo = 0;
#endif
}
#endif
