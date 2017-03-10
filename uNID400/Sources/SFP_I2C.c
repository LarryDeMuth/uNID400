/*
 * NetI2C.c
 *
 *  Created on: Apr 4, 2015
 *      Author: Larry
 */
#include <mqx.h>
#include <string.h>
#include "extern.h"
#include "config.h"

extern void LogEvent(uint_8 event, uint_8 source);
extern void DefaultSFPData(uint_8 side);
extern void DefaultSFPDataA2(uint_8 side);
extern void GetA2Values(uint_8 side);

static void ReadSFPI2C(uint_8 sfp);
static uint_8 GetSFPA0(uint_8 sfp);
static uint_8 GetSFPA2(uint_8 sfp);
static void ResetI2C();
static bool SetMux(uint_8 sfp);

void ReadSFPModule(uint_8 sfp);
bool ChangeTXDisable(uint_8 sfp, uint_8 disable);
bool ChangeRXRate(uint_8 sfp);
bool ChangeTXRate(uint_8 sfp);
bool ChangePower(uint_8 sfp);

void ReadSFPModule(uint_8 sfp){
	struct sfpdata *data;

	if (sfp < SOURCE_NET_P_SFP|| sfp > SOURCE_CUST_S_SFP)
		return;
	data = &SFPStatus[sfp];

	if (NoSFP[sfp]){											//no sfp plugged in
		if (data->SFPFlags & SFP_EVENT_SFP_IN){							//check our events
			LogEvent(EVENT_SFP_REMOVED, sfp);
			data->SFPFlags &= ~(SFP_EVENT_SFP_IN | SFP_EVENT_COMM_OK);//comm is lost since no sfp, but dont log it, just clear the flag
			if (!(data->SFPFlags & SFP_EVENT_LOS) && ILOS[sfp]){
				LogEvent(EVENT_SIGNAL_LOS, sfp);
				data->SFPFlags |= SFP_EVENT_LOS;
			}
		}
		if (data->SFPFlags & DDM_IMPLEMENTED){						//if a2 implemented
			DefaultSFPDataA2(sfp);					//default it
			data->ReadFail = 0;						//reset our fail counter
		}
		if (data->SFPFlags & A0_READ){								//if no SFP and A0 was read
			data->SFPFlags &= ~A0_READ;
			DefaultSFPData(sfp);					//default it
		}
		if (sfp == SOURCE_NET_P_SFP)
			LEDState[NET_P_PROB] = LED_OFF;
		else if (sfp == SOURCE_NET_S_SFP)
			LEDState[NET_S_PROB] = LED_OFF;
		else if (sfp == SOURCE_CUST_P_SFP)
			LEDState[CUST_P_PROB] = LED_OFF;
		else
			LEDState[CUST_S_PROB] = LED_OFF;
	}
	else{							//if Net SFP plugged in, check our events
		if (!(data->SFPFlags & SFP_EVENT_SFP_IN)){
			LogEvent(EVENT_SFP_INSTALLED, sfp);
			data->SFPFlags |= SFP_EVENT_SFP_IN;
		}
		if ((data->SFPFlags & SFP_EVENT_LOS) && !ILOS[sfp]){
			LogEvent(EVENT_SIGNAL_OK, sfp);
			data->SFPFlags &= ~SFP_EVENT_LOS;
		}
		else if (!(data->SFPFlags & SFP_EVENT_LOS) && ILOS[sfp]){
			LogEvent(EVENT_SIGNAL_LOS, sfp);
			data->SFPFlags |= SFP_EVENT_LOS;
		}
		ReadSFPI2C(sfp);								//read the registers
	}
}

static void ReadSFPI2C(uint_8 sfp){
	struct sfpdata *data;

	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return;
	data = &SFPStatus[sfp];

	if (!(data->SFPFlags & A0_READ)){					//if we havent read A0 yet
		if (GetSFPA0(sfp)){				//if error reading A0 data
			ResetI2C();					//reset the i2c bus
			SFPError = FALSE;			//clear the timeout error
			DefaultSFPData(sfp); 		//default the A0 structure
			DefaultSFPDataA2(sfp); 		//default the A2 structure
			if (data->SFPFlags & SFP_EVENT_COMM_OK){
				LogEvent(EVENT_SFP_COMM_LOST, sfp);
				data->SFPFlags &= ~SFP_EVENT_COMM_OK;
			}
		}
		else{							//if no errors reading it
			data->SFPFlags |= A0_READ;			//flag it read
			if (sfp == SOURCE_NET_P_SFP)
				LEDState[NET_P_PROB] = LED_OFF;
			else if (sfp == SOURCE_NET_S_SFP)
				LEDState[NET_S_PROB] = LED_OFF;
			else if (sfp == SOURCE_CUST_P_SFP)
				LEDState[CUST_P_PROB] = LED_OFF;
			else
				LEDState[CUST_S_PROB] = LED_OFF;
			if (!(data->SFPFlags & SFP_EVENT_COMM_OK)){
				LogEvent(EVENT_SFP_COMM_OK, sfp);
				data->SFPFlags |= SFP_EVENT_COMM_OK;
			}
		}
	}
	if ((data->SFPFlags & (A0_READ | DDM_IMPLEMENTED)) == (A0_READ | DDM_IMPLEMENTED)){	//if we read A0 and A2 is supported
		if (GetSFPA2(sfp)){					//if error reading A2 data
			SFPError = FALSE;				//reset timeout error
			ResetI2C();						//reset the i2c bus
			data->ReadFail++;
			if (data->ReadFail == 3){			//if 3 fails in a row
				data->ReadFail = 0;			//reset fail counter
				DefaultSFPDataA2(sfp); 		//default the actual readings
				data->SFPFlags &= ~A0_READ;	//start all over with reading A0
				if (data->SFPFlags & SFP_EVENT_A2_OK){
					LogEvent(EVENT_ENHANCED_FAIL, sfp);
					data->SFPFlags &= ~SFP_EVENT_A2_OK;
				}
			}
		}
		else{								//if good reading
			GetA2Values(sfp);				//get the actual values
			data->ReadFail = 0;				//reset fail counter
			data->SFPFlags &= ~FIRST_READ_A2;			//we got the constants so no need to read them every time
			if (!(data->SFPFlags & SFP_EVENT_A2_OK)){
				LogEvent(EVENT_ENHANCED_OK, sfp);
				data->SFPFlags |= SFP_EVENT_A2_OK;
			}
		}
	}
}

static uint_8 GetSFPA0(uint_8 sfp){
	LDD_I2C_TBusState BusState;
	uint_8 error = 0;
	unsigned char OutBuffer[2];
	struct sfpdata *data;
	struct SFPDataA0 *DataA0;

	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return 0;
	data = &SFPStatus[sfp];
	DataA0 = &SFPDataA0[sfp];

	do{
		SFPs_I2C_CheckBus(SFPPtr, &BusState);				//wait for bus to be idle
		error++;										//but dont wait forever...
	}
	while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
	if (BusState != LDD_I2C_BUSY){						//if bus is idle...
		if (!SetMux(sfp))
			return 1;
		if (data->SFPFlags & ADDR_CHANGE){								//if we had to do the funky address change...
			if (SFPs_I2C_SelectSlaveDevice(SFPPtr, LDD_I2C_ADDRTYPE_7BITS, PAGE_CHANGE_ADDRESS) != ERR_OK) //can we select the change address?
				return 2;								//if not then return an error
			OutBuffer[0] = SFP_ADDR_CHANGE_REG;			//the register for address change
			OutBuffer[1] = SFP_ADDRESS << 1;			//the address to change to
			if (SFPs_I2C_MasterSendBlock(SFPPtr, OutBuffer, 2, LDD_I2C_SEND_STOP) != ERR_OK)	//did it write out?
				return 3;								//if not, return an error
			SFPDataTransmittedFlg = FALSE;
			while (!SFPDataTransmittedFlg){				//wait till done writing or a timeout
			}
			if (SFPError)								//did it timeout?
				return 5;								//return an error
			error = 0;
			do{
				SFPs_I2C_CheckBus(SFPPtr, &BusState);		//wait until the bus goes idle
				error++;								//but not forever...
			}
			while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
			if (BusState == LDD_I2C_BUSY)				//is the bus still busy?
				return 1;								//return an error
		}
		if (SFPs_I2C_SelectSlaveDevice(SFPPtr, LDD_I2C_ADDRTYPE_7BITS, SFP_ADDRESS) != ERR_OK) //select the address
			return 2;									//return an error if we couldnt
		OutBuffer[0] = SFP_IDENTIFIER;						//starting address to read
		if (SFPs_I2C_MasterSendBlock(SFPPtr, OutBuffer, 1, LDD_I2C_NO_SEND_STOP) != ERR_OK)		//write address to read
			return 3;									//return an error if it failed
		SFPDataTransmittedFlg = FALSE;
		while (!SFPDataTransmittedFlg){					//wait until done or timeout
		}
		if (SFPError)									//if a timeout
			return 5;									//return an eror
		if (SFPs_I2C_MasterReceiveBlock(SFPPtr, (LDD_TData*)&DataA0, sizeof(struct SFPDataA0), LDD_I2C_SEND_STOP) != ERR_OK)//read the data
			return 4;									//if error starting the read, return an error
		SFPDataReceivedFlg = FALSE;
		while (!SFPDataReceivedFlg){					//wait until read is done or timeout
		}
		if (SFPError)									//if timeout...
			return 5;									//return an error
		data->SFPType = SFP_NORMAL;
		if (DataA0->DiagMonType & ADDRESS_CHANGE_REQUIRED)
			data->SFPFlags |= ADDR_CHANGE;
		else
			data->SFPFlags &= ~ADDR_CHANGE;
		if (DataA0->DiagMonType & SFP_DDM_IMPLEMENTED)
			data->SFPFlags |= DDM_IMPLEMENTED;
		else
			data->SFPFlags &= ~DDM_IMPLEMENTED;
		switch (DataA0->RateIdentifier){
			case 1:			//not sure about this one
			case 6:
			case 10:
			case 12:
			case 14:
				data->Options = RX_RS | TX_RS;
			break;
			case 2:
			case 8:
				data->Options = RX_RS;
			break;
			case 4:
				data->Options = TX_RS;
			break;
			default:
				data->Options = 0;
			break;
		}
		if (DataA0->Options[1] & RATE_SELECT_IMPLEMENTED)
			data->Options |= RX_RS;
		if (DataA0->Options[1] & TX_DISABLE_IMPLEMENTED)
			data->Options |= TX_DISABLE;
		if (DataA0->Options[1] & TX_FAULT_IMPLEMENTED)
			data->Options |= TX_FAULT;
		if (DataA0->EnhancedOptions & (SOFT_RATE_C_M_IMPLEMEMTED | RATE_SELECT_C_IMPLEMENTED)){
			if (data->Options & RX_RS)
				data->Options |= RX_SOFT_RS;
			if (data->Options & TX_RS)
				data->Options |= TX_SOFT_RS;
		}
		if (DataA0->EnhancedOptions & TX_DISABLE_C_M_IMPLEMENTED)
			data->Options |= TX_SOFT_DISABLE;
		if (DataA0->EnhancedOptions & TX_FAULT_M_IMPLEMENTED)
			data->Options |= TX_SOFT_FAULT;

		if (!(data->SFPFlags & DDM_IMPLEMENTED)){		//if A0 registers only
			if (sfp == SOURCE_NET_P_SFP || sfp == SOURCE_NET_S_SFP)
				ChangeTXDisable(sfp, *NetTXDisableState);						//set the disabled state
			else
				ChangeTXDisable(sfp, *CustTXDisableState);						//set the disabled state
			ChangeRXRate(sfp);
		}
		data->SpeedH = data->SpeedL = LED_OFF;
		if (DataA0->Tranceiver[0] & 0xf0)		//if 10G rate
			data->SpeedH = LED_BLINK;							//the highest rate
		else if (DataA0->Tranceiver[0] == 0){		//if none, check BRNom reg for 10G
			if (DataA0->BRNominal > 90 && DataA0->BRNominal < 120) //if we have a bit rate less than say 12G, we are 10G
				data->SpeedH = LED_BLINK;							//the highest rate
		}
		if (DataA0->Tranceiver[3] & 0x0d){		//if 1G rate
			if (data->SpeedH == 0)					//if no high speed
				data->SpeedH = LED_PULSE;						//this is our high speed
			else
				data->SpeedL = LED_PULSE;
		}
		if ((DataA0->Tranceiver[3] & 0xc2) || DataA0->Tranceiver[3] == 0){//dont know rate, must look at BRnom
			if (DataA0->BRNominal > 9 && DataA0->BRNominal < 20){ //if we have a bit rate less than say 2G, we are 1G
				if (data->SpeedH == 0)					//if no high speed
					data->SpeedH = LED_PULSE;						//this is our high speed
				else
					data->SpeedL = LED_PULSE;
			}
			else if (DataA0->BRNominal == 1){	//if less than 1G, we are 100M
				if (data->SpeedH == 0)					//if no high speed
					data->SpeedH = LED_ON;						//this is our high speed
				else
					data->SpeedL = LED_ON;
			}
		}
		if (DataA0->Tranceiver[3] & 0x30){	//if 100M rate
			if (data->SpeedH == 0)					//if no high speed
				data->SpeedH = LED_ON;						//this is our high speed
			else
				data->SpeedL = LED_ON;
		}
		if (sfp == SOURCE_NET_P_SFP){
			if ((data->Options & RX_RS) && *NetRXRateState == LOW_RATE_RX) //if rate select, and low rate selected
				LEDState[NET_P_SPEED] = data->SpeedL;
			else										//if no rate select, or high rate selected
				LEDState[NET_P_SPEED] = data->SpeedH;
		}
		else if (sfp == SOURCE_NET_S_SFP){
			if ((data->Options & RX_RS) && *NetRXRateState == LOW_RATE_RX) //if rate select, and low rate selected
				LEDState[NET_S_SPEED] = data->SpeedL;
			else										//if no rate select, or high rate selected
				LEDState[NET_S_SPEED] = data->SpeedH;
		}
		else if (sfp == SOURCE_CUST_P_SFP){
			if ((data->Options & RX_RS) && *CustRXRateState == LOW_RATE_RX) //if rate select, and low rate selected
				LEDState[CUST_P_SPEED] = data->SpeedL;
			else										//if no rate select, or high rate selected
				LEDState[CUST_P_SPEED] = data->SpeedH;
		}
		else{
			if ((data->Options & RX_RS) && *CustRXRateState == LOW_RATE_RX) //if rate select, and low rate selected
				LEDState[CUST_S_SPEED] = data->SpeedL;
			else										//if no rate select, or high rate selected
				LEDState[CUST_S_SPEED] = data->SpeedH;
		}
	}
	else
		return 1;										//if bus was still busy, return an error
	return 0;											//A0 read successfully
}

static uint_8 GetSFPA2(uint_8 sfp){
	LDD_I2C_TBusState BusState;
	unsigned char OutBuffer[2];
	uint_8 error = 0, *ptr, size;
	struct sfpdata *data;
	struct SFPDataA2 *DataA2;

	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return 0;
	data = &SFPStatus[sfp];
	DataA2 = &SFPDataA2[sfp];

	do{
		SFPs_I2C_CheckBus(SFPPtr, &BusState);				//wait for bus to go idle
		error++;										//but dont wait forever...
	}
	while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
	if (BusState != LDD_I2C_BUSY){						//if bus is idle
		if (!SetMux(sfp))
			return 1;
		if (data->SFPFlags & ADDR_CHANGE){								//if we have to do the funky address change...
			if (SFPs_I2C_SelectSlaveDevice(SFPPtr, LDD_I2C_ADDRTYPE_7BITS, PAGE_CHANGE_ADDRESS) != ERR_OK)	//select the address for changing
				return 2;								//if fail, return an error
			OutBuffer[0] = SFP_ADDR_CHANGE_REG;			//select the change register
			OutBuffer[1] = SFP_PLUS_ADDRESS << 1;		//and the address to change to
			if (SFPs_I2C_MasterSendBlock(SFPPtr, OutBuffer, 2, LDD_I2C_SEND_STOP) != ERR_OK)	//write it out
				return 3;								//if error starting write, return an error
			SFPDataTransmittedFlg = FALSE;
			while (!SFPDataTransmittedFlg){				//wait till write completes or timeout
			}
			if (SFPError)								//if timeout
				return 5;								//return an error
			error = 0;
			do{
				SFPs_I2C_CheckBus(SFPPtr, &BusState);		//wait till bus goes idle
				error++;								//but dont wait forever...
			}
			while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
			if (BusState == LDD_I2C_BUSY)				//if bus is still busy
				return 1;								//return an error
		}
		if (SFPs_I2C_SelectSlaveDevice(SFPPtr, LDD_I2C_ADDRTYPE_7BITS, SFP_PLUS_ADDRESS) != ERR_OK) //select the A2 address
			return 2;									//if we couldnt, return an error
		if (data->SFPFlags & FIRST_READ_A2){							//if our first time reading A2
			OutBuffer[0] = AW_THRESHOLDS;				//read entire structure
			ptr = (uint_8*)&SFPDataA2[sfp];
			size = sizeof(struct SFPDataA2);
		}
		else{											//if not
			OutBuffer[0] = DIAGNOSTICS;					//just get the values and alarms
			ptr = &SFPDataA2[sfp].IntTemp[0];
			size = sizeof(struct SFPDataA2) - DIAGNOSTICS;
		}
		if (SFPs_I2C_MasterSendBlock(SFPPtr, OutBuffer, 1, LDD_I2C_NO_SEND_STOP) != ERR_OK)		//send it
			return 3;									//if error starting send, reeturn an error
		SFPDataTransmittedFlg = FALSE;
		while (!SFPDataTransmittedFlg){					//wait for send to complete or timeout
		}
		if (SFPError)									//if timeout
			return 5;									//return an error
		if (SFPs_I2C_MasterReceiveBlock(SFPPtr, (LDD_TData*)ptr, size, LDD_I2C_SEND_STOP) != ERR_OK) //start read
			return 4;									//if error starting read, return an error
		SFPDataReceivedFlg = FALSE;
		while (!SFPDataReceivedFlg){					//wait for read to finish or timeout
		}
		if (SFPError)									//if timeout
			return 5;									//return an error
		if ((data->SFPFlags & (FIRST_READ_A2 | DDM_IMPLEMENTED)) == (FIRST_READ_A2 | DDM_IMPLEMENTED)){
				//if the first read, set defaults if applicable
			ChangePower(sfp);
			ChangeRXRate(sfp);
			ChangeTXRate(sfp);
			if (sfp == SOURCE_NET_P_SFP || sfp == SOURCE_NET_S_SFP)
				ChangeTXDisable(sfp, *NetTXDisableState);
			else
				ChangeTXDisable(sfp, *CustTXDisableState);
		}
		//if disable and fault implemented
		if (data->Options & (TX_SOFT_DISABLE | TX_SOFT_FAULT) == (TX_SOFT_DISABLE | TX_SOFT_FAULT)){
			if (DataA2->StatusControl & TX_FAULT_PIN_STATE){			//if tx fault (it latches)
				if (!(data->SFPFlags & SFP_EVENT_TX_FAULT)){// || NetClearFault){						//if no fault logged, or trying to clear a fault
					if (data->FaultCount < 3){									//try 3 times
						_time_delay(2);
						ChangeTXDisable(sfp, DISABLE_TX);							//disable tx
						_time_delay(10);
						ChangeTXDisable(sfp, ENABLE_TX);								//enable tx
						data->FaultCount++;
					}
					else{													//if 3 tries
						if (!(data->SFPFlags & SFP_EVENT_TX_FAULT)){								//if we havent logged it yet
							LogEvent(EVENT_TX_FAULT, sfp);		//log it
							data->SFPFlags |= SFP_EVENT_TX_FAULT;
						}
//						NetClearFault = FALSE;
						data->FaultCount = 0;
					}
				}
			}
			else if (data->SFPFlags & SFP_EVENT_TX_FAULT){										//if no fault and we logged a fault
				LogEvent(EVENT_TX_FAULT_CLEAR, sfp);				//log cleared
				data->SFPFlags &= ~SFP_EVENT_TX_FAULT;
//				NetClearFault = FALSE;
				data->FaultCount = 0;
			}
		}
	}
	else												//if bus was still busy
		return 1;										//return an error
	return 0;											//successful read
}

bool ChangePower(uint_8 sfp){
	LDD_I2C_TBusState BusState;
	unsigned char OutBuffer[2];
	uint_8 error = 0;
	struct sfpdata *data;
	struct SFPDataA2 *DataA2;
	struct SFPDataA0 *DataA0;
	char *option;

	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return 1;
	data = &SFPStatus[sfp];
	DataA2 = &SFPDataA2[sfp];
	DataA0 = &SFPDataA0[sfp];

	if (!(data->SFPFlags & DDM_IMPLEMENTED) || !(DataA0->Options[0] & ((POWER_LEVEL_LOW | POWER_LEVEL_HIGH) >> 8)))
		return 1;
	if (sfp == SOURCE_NET_P_SFP || sfp == SOURCE_NET_S_SFP)
		option = NetTXPowerState;
	else
		option = CustTXPowerState;
	_time_delay(2);
	do{
		SFPs_I2C_CheckBus(SFPPtr, &BusState);				//wait for bus to go idle
		error++;										//but dont wait forever...
	}
	while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
	if (BusState == LDD_I2C_BUSY)						//if bus is not idle
		return 0;
	if (!SetMux(sfp))
		return 0;
	if (SFPs_I2C_SelectSlaveDevice(SFPPtr, LDD_I2C_ADDRTYPE_7BITS, SFP_PLUS_ADDRESS) != ERR_OK)
		return 0;								//if fail, return an error
	OutBuffer[0] = EXT_STATUS_CONTROL;
	if (*option == LOW_POWER)
		OutBuffer[1] = DataA2->ExtStatusControl[0] &= ~(POWER_LEVEL_SELECT >> 8);
	else if (*option == HIGH_POWER)
		OutBuffer[1] = DataA2->ExtStatusControl[0] | (POWER_LEVEL_SELECT >> 8);
	else
		return 1;
	if (SFPs_I2C_MasterSendBlock(SFPPtr, OutBuffer, 2, LDD_I2C_SEND_STOP) != ERR_OK)	//write it out
		return 0;								//if error starting write, return an error
	SFPDataTransmittedFlg = FALSE;
	while (!SFPDataTransmittedFlg){				//wait till write completes or timeout
	}
	if (SFPError)									//if timeout...
		return 0;									//return an error
	return 1;
}

bool ChangeRXRate(uint_8 sfp){
	uint_8 tmp;

	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return 1;
	if (!(SFPStatus[sfp].Options & RX_RS))		//if RX rate select not implemented
		return 1;								//we are done
	if (sfp == SOURCE_NET_P_SFP){
		if (*NetRXRateState == LOW_RATE_RX){
			PortC_ClearFieldBits(PortC_DeviceData, NP_Pins, RATE_SELECT_PIN);
			LEDState[NET_P_SPEED] = SFPStatus[sfp].SpeedL;
		}
		else if (*NetRXRateState == HIGH_RATE_RX){
			PortC_SetFieldBits(PortC_DeviceData, NP_Pins, RATE_SELECT_PIN);
			LEDState[NET_P_SPEED] = SFPStatus[sfp].SpeedH;
		}
	}
	else if (sfp == SOURCE_NET_S_SFP){
		if (*NetRXRateState == LOW_RATE_RX){
			PortC_ClearFieldBits(PortC_DeviceData, NS_Pins, RATE_SELECT_PIN);
			LEDState[NET_S_SPEED] = SFPStatus[sfp].SpeedL;
		}
		else if (*NetRXRateState == HIGH_RATE_RX){
			PortC_SetFieldBits(PortC_DeviceData, NS_Pins, RATE_SELECT_PIN);
			LEDState[NET_S_SPEED] = SFPStatus[sfp].SpeedH;
		}
	}
	else if (sfp == SOURCE_CUST_P_SFP){
		if (*CustRXRateState == LOW_RATE_RX){
			PortC_ClearFieldBits(PortC_DeviceData, CP_Pins, RATE_SELECT_PIN);
			LEDState[CUST_P_SPEED] = SFPStatus[sfp].SpeedL;
		}
		else if (*CustRXRateState == HIGH_RATE_RX){
			PortC_SetFieldBits(PortC_DeviceData, CP_Pins, RATE_SELECT_PIN);
			LEDState[CUST_P_SPEED] = SFPStatus[sfp].SpeedH;
		}
	}
	else{
		if (*CustRXRateState == LOW_RATE_RX){
			PortC_ClearFieldBits(PortC_DeviceData, CS_Pins, RATE_SELECT_PIN);
			LEDState[CUST_S_SPEED] = SFPStatus[sfp].SpeedL;
		}
		else if (*CustRXRateState == HIGH_RATE_RX){
			PortC_SetFieldBits(PortC_DeviceData, CS_Pins, RATE_SELECT_PIN);
			LEDState[CUST_S_SPEED] = SFPStatus[sfp].SpeedH;
		}
	}
	return 1;
}

bool ChangeTXRate(uint_8 sfp){
	LDD_I2C_TBusState BusState;
	unsigned char OutBuffer[2];
	uint_8 error = 0;
	char *option;

	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return 1;

	if (!(SFPStatus[sfp].Options & TX_SOFT_RS))	//if no soft controls
		return 1;													//just return
	if (sfp == SOURCE_NET_P_SFP || sfp == SOURCE_NET_S_SFP)
		option = NetTXRateState;
	else
		option = CustTXRateState;
	_time_delay(2);
	do{
		SFPs_I2C_CheckBus(SFPPtr, &BusState);				//wait for bus to go idle
		error++;										//but dont wait forever...
	}
	while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
	if (BusState == LDD_I2C_BUSY)						//if bus is not idle
		return 0;
	if (!SetMux(sfp))
		return 0;
	if (SFPs_I2C_SelectSlaveDevice(SFPPtr, LDD_I2C_ADDRTYPE_7BITS, SFP_PLUS_ADDRESS) != ERR_OK)
		return 0;								//if fail, return an error
	if (*option == LOW_RATE_TX){
		OutBuffer[0] = EXT_STATUS_CONTROL;
		OutBuffer[1] = SFPDataA2[sfp].ExtStatusControl[0] &= ~(SOFT_RS1_RATE_SELECT >> 8);
	}
	else if (*option == HIGH_RATE_TX){
		OutBuffer[0] = EXT_STATUS_CONTROL;
		OutBuffer[1] = SFPDataA2[sfp].ExtStatusControl[0] | (SOFT_RS1_RATE_SELECT >> 8);
	}
	else
		return 1;
	if (SFPs_I2C_MasterSendBlock(SFPPtr, OutBuffer, 2, LDD_I2C_SEND_STOP) != ERR_OK)	//write it out
		return 0;								//if error starting write, return an error
	SFPDataTransmittedFlg = FALSE;
	while (!SFPDataTransmittedFlg){				//wait till write completes or timeout
	}
	if (SFPError)									//if timeout...
		return 0;									//return an error
	return 1;
}

bool ChangeTXDisable(uint_8 sfp, uint_8 disable){
	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return 1;
	if (!(SFPStatus[sfp].Options & TX_DISABLE))
		return 1;
	if (sfp == SOURCE_NET_P_SFP){
		if (disable == ENABLE_TX)		//also set the pin
			PortC_ClearFieldBits(PortC_DeviceData, NP_Pins, TX_DISABLE_PIN);
		else
			PortC_SetFieldBits(PortC_DeviceData, NP_Pins, TX_DISABLE_PIN);
	}
	else if (sfp == SOURCE_NET_S_SFP){
		if (disable == ENABLE_TX)		//also set the pin
			PortC_ClearFieldBits(PortC_DeviceData, NS_Pins, TX_DISABLE_PIN);
		else
			PortC_SetFieldBits(PortC_DeviceData, NS_Pins, TX_DISABLE_PIN);
	}
	else if (sfp == SOURCE_CUST_P_SFP){
		if (disable == ENABLE_TX)		//also set the pin
			PortC_ClearFieldBits(PortC_DeviceData, CP_Pins, TX_DISABLE_PIN);
		else
			PortC_SetFieldBits(PortC_DeviceData, CP_Pins, TX_DISABLE_PIN);
	}
	else if (sfp == SOURCE_CUST_S_SFP){
		if (disable == ENABLE_TX)		//also set the pin
			PortC_ClearFieldBits(PortC_DeviceData, CS_Pins, TX_DISABLE_PIN);
		else
			PortC_SetFieldBits(PortC_DeviceData, CS_Pins, TX_DISABLE_PIN);
	}
	return 1;
}

static bool SetMux(uint_8 sfp){
	LDD_I2C_TBusState BusState;
	unsigned char OutBuffer;
	uint_8 error = 0;

	if (sfp < SOURCE_NET_P_SFP || sfp > SOURCE_CUST_S_SFP)
		return 1;
	if (SelectedSFP == sfp)
		return 1;
	_time_delay(2);
	do{
		SFPs_I2C_CheckBus(SFPPtr, &BusState);				//wait for bus to go idle
		error++;										//but dont wait forever...
	}
	while(BusState == LDD_I2C_BUSY && error < MAX_BUSY_WAIT);
	if (BusState == LDD_I2C_BUSY)						//if bus is not idle
		return 0;
	if (SFPs_I2C_SelectSlaveDevice(SFPPtr, LDD_I2C_ADDRTYPE_7BITS, SFP_MUX_ADDRESS) != ERR_OK)
		return 0;								//if fail, return an error
	OutBuffer = sfp | 0x4;
	if (SFPs_I2C_MasterSendBlock(SFPPtr, &OutBuffer, 1, LDD_I2C_SEND_STOP) != ERR_OK)	//write it out
		return 0;								//if error starting write, return an error
	SFPDataTransmittedFlg = FALSE;
	while (!SFPDataTransmittedFlg){				//wait till write completes or timeout
	}
	if (SFPError)									//if timeout...
		return 0;									//return an error
	SelectedSFP = sfp;
	return 1;
}

static void ResetI2C(){
	uint_8 a, delay;
	GPIO_MemMapPtr ptd_ptr = PTA_BASE_PTR;

	SFPs_I2C_Deinit(SFPPtr);
	ptd_ptr->PDDR &= (uint32_t)~(uint32_t)(SFP_SCL);	//make input
	ptd_ptr->PDOR |= SFP_SCL;							//for when its an output... make scl 1
	ptd_ptr->PDDR &= (uint32_t)~(uint32_t)(SFP_SDA);	//make input
	PORTA_PCR11 = ((PORTA_PCR11 & ~(PORT_PCR_ISF_MASK | PORT_PCR_MUX(0x07))) | (PORT_PCR_MUX(0x01)));
	PORTA_PCR12 = ((PORTA_PCR12 & ~(PORT_PCR_ISF_MASK | PORT_PCR_MUX(0x07))) | (PORT_PCR_MUX(0x01)));
	if ((ptd_ptr->PDIR & (SFP_SDA | SFP_SCL)) != (SFP_SDA | SFP_SCL)){		//if scl or sda low
		for (delay = 0; delay < 50; delay++){
		}
		if (!(ptd_ptr->PDIR & SFP_SCL)){
			ptd_ptr->PDDR |= SFP_SCL; //set as output
			for (a = 0; a < 9; a++){
				ptd_ptr->PCOR |= SFP_SCL;	//clear scl
				for (delay = 0; delay < 25; delay++){
				}
				ptd_ptr->PSOR |= SFP_SCL;	//set scl
				for (delay = 0; delay < 25; delay++){
				}
				if (ptd_ptr->PDIR & SFP_SDA)	//is SDA set?
					break;
			}
		}
	}
	ptd_ptr->PDDR &= (uint32_t)~(uint32_t)(SFP_SCL);	//make input
	ptd_ptr->PDOR |= SFP_SCL;							//for when its an output... make scl 1
	ptd_ptr->PDDR &= (uint32_t)~(uint32_t)(SFP_SDA);	//make input
	SFPPtr = SFPs_I2C_Init(NULL);
}

