/*
 * BkplnTask.c
 *
 *  Created on: Dec 27, 2016
 *      Author: Larry
 */

#include "config.h"
#include <mqx.h>
#include <bsp.h>
#include "extern.h"
#include <string.h>
#include "bkpln.h"

slavestate SlaveStatus;
addresscode AddressByte;
uint_16 RcvByteCount, XmitByteCount;
uint_8 RcvBuffer[32], XmitBuffer[256];
bool StopBit;
uint_8 tmpevents;

/*	I2C BACKPLANE OPERATIONS
 * S = START BIT, T = STOP BIT, R = R/W BIT (W is low, R is high), A = ACK BIT
 * COMMAND WRITE:
 *          S SLOT ADDRESS R A   COMMAND      A   BYTE 1       A      BYTE 2    A       BYTE n       A T
 * SDA Line []_|_[][][][][]L|_[][][][][][][][]_[][][][][][][][]_[][][][][][][][]_ // [][][][][][][][]_[]
 *
 * COMMAND READ:
 *          S SLOT ADDRESS R A   COMMAND      A S SLOT ADDRESS R A   BYTE 1       A      BYTE 2    A       BYTE n       A T
 * SDA Line []_|_[][][][][]L|_[][][][][][][][]_|[]_|_[][][][][]H|_[][][][][][][][]_[][][][][][][][]_ // [][][][][][][][]_[]
 *
 * Current read IS allowed, but not recommended. The command last read/written will be the command read.
 */
void BkPln_I2C_OnSlaveByteReceived(LDD_TUserData *UserDataPtr)
{
	if (RcvByteCount == 0)
		AddressByte = RcvBuffer[0];
	RcvByteCount++;
}

void BkPln_I2C_OnSlaveRxRequest(LDD_TUserData *UserDataPtr)
{
	SlaveStatus = SLAVE_RX;
	RcvByteCount = 0;
	BkPln_I2C_SlaveReceiveBlock(BkPlnPtr, RcvBuffer, 32);
}

void BkPln_I2C_OnSlaveTxRequest(LDD_TUserData *UserDataPtr)
{
	uint_8 a, b;
	uint_8 *ptr;

	SlaveStatus = SLAVE_TX;
	switch(AddressByte){
		case SLOT_INFO_ADDRESS:				//47 bytes
			b = 0;
			strcpy(&XmitBuffer[b], ProductString);
			b += strlen(ProductString);
			for (; b < 34; b++)						//32 bytes max + null
				XmitBuffer[b] = 0;
			strcpy(&XmitBuffer[b], SWREV);
			b += strlen(SWREV);
			for (; b < 45; b++)
				XmitBuffer[b] = 0;					//10 bytes max + null
			XmitBuffer[b++] = 1;		 			//net primary sfp installed		MUST BE IN THIS ORDER!!!
			XmitBuffer[b++] = 1;		 			//net secondary sfp installed
			XmitBuffer[b++] = 1;		 			//cust secondary sfp installed
			XmitBuffer[b++] = 1;		 			//cust primary sfp installed
			XmitByteCount = b;
		break;
		case SFP_SUM_STATUS_ADDRESS:				//32 bytes
			b = 0;
			for (a = 0; a < 4; a++){
				XmitBuffer[b++] = NoSFP[a];
				XmitBuffer[b++] = ILOS[a];
				XmitBuffer[b++] = Fault[a];
				XmitBuffer[b++] = SFPStatus[a].SFPType;
				XmitBuffer[b++] = SFPStatus[a].SFPFlags >> 8;
				XmitBuffer[b++] = SFPStatus[a].SFPFlags;
				if (a == 0){
					XmitBuffer[b++] = LEDState[NET_P_PROB];
					XmitBuffer[b++] = LEDState[NET_P_SPEED];
				}
				else if (a == 1){
					XmitBuffer[b++] = LEDState[NET_S_PROB];
					XmitBuffer[b++] = LEDState[NET_S_SPEED];
				}
				else if (a == 2){
					XmitBuffer[b++] = LEDState[CUST_S_PROB];
					XmitBuffer[b++] = LEDState[CUST_S_SPEED];
				}
				else{
					XmitBuffer[b++] = LEDState[CUST_P_PROB];
					XmitBuffer[b++] = LEDState[CUST_P_SPEED];
				}
			}
			XmitByteCount = b;
		break;
		case SFP_ID_ADDRESS:								//216 bytes
			b = 0;
			for (a = 0; a < 4; a++){
				XmitBuffer[b++] = SFPDataA0[a].SFF_8472Compilance;
				strncpy(&XmitBuffer[b], SFPDataA0[a].VendorName, 16);		//16
				b += 16;
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[0];
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[1];
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[2];
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[3];
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[4];
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[5];
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[6];
				XmitBuffer[b++] = SFPDataA0[a].Tranceiver[7];
				strncpy(&XmitBuffer[b], SFPDataA0[a].VendorPN, 16);		//16
				b += 16;
				strncpy(&XmitBuffer[b], SFPDataA0[a].VendorRev, 4);		//4
				b += 4;
				XmitBuffer[b++] = SFPDataA0[a].Connector;
				XmitBuffer[b++] = SFPDataA0[a].Encoding;
				XmitBuffer[b++] = SFPDataA0[a].BRNominal;
				XmitBuffer[b++] = SFPDataA0[a].LengthSMFkm;
				XmitBuffer[b++] = SFPDataA0[a].LengthSMF;
				XmitBuffer[b++] = SFPDataA0[a].Length50um;
				XmitBuffer[b++] = SFPDataA0[a].LengthOM3;
				XmitBuffer[b++] = SFPDataA0[a].Length62_5um;
				XmitBuffer[b++] = SFPDataA0[a].LengthCable;
			}
			XmitByteCount = b;
		break;
		case SFP_STATUS_ADDRESS:								//128 bytes
			b = 0;
			for (a = 0; a < 4; a++){
				XmitBuffer[b++] = SFPStatus[a].EventAlarmFlags >> 8;
				XmitBuffer[b++] = SFPStatus[a].EventAlarmFlags;
				XmitBuffer[b++] = SFPStatus[a].EventWarnFlags >> 8;
				XmitBuffer[b++] = SFPStatus[a].EventWarnFlags;
				ptr = (uint_8*)&ActualValues[a].IntTemp;	//float values
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				ptr = (uint_8*)&ActualValues[a].Voltage;	//float values
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				ptr = (uint_8*)&ActualValues[a].Bias;	//float values
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				ptr = (uint_8*)&ActualValues[a].TXPower;	//float values
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				ptr = (uint_8*)&ActualValues[a].RXPower;	//float values
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				ptr = (uint_8*)&ActualValues[a].LaserTemp;	//float values
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				ptr = (uint_8*)&ActualValues[a].TECCurrent;	//float values
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
				XmitBuffer[b++] = *ptr++;
			}
			XmitByteCount = b;
		break;
		case SFP_PROV_ADDRESS:							//12 bytes
			b = 0;
			XmitBuffer[b++] = *NetTXDisableState;
			XmitBuffer[b++] = *NetTXRateState;
			XmitBuffer[b++] = *NetTXPowerState;
			XmitBuffer[b++] = *NetRXRateState;
			XmitBuffer[b++] = *CustTXDisableState;
			XmitBuffer[b++] = *CustTXRateState;
			XmitBuffer[b++] = *CustTXPowerState;
			XmitBuffer[b++] = *CustRXRateState;
			for (a = 0; a < 4; a++){
				XmitBuffer[b++] = SFPStatus[a].Options;
			}
			XmitByteCount = b;
		break;
		case EVENT_COUNT_ADDRESS:							//1 byte
			XmitBuffer[0] = NumEvents;
			tmpevents = NumEvents;
			XmitByteCount = 1;
		break;
		case EVENTS_ADDRESS:								//tmpevents * 2 bytes
			b = 0;
			for (a = 0; a < tmpevents; a++){
				XmitBuffer[b++] = WorkingLog[FirstEvent].Source;
				XmitBuffer[b++] = WorkingLog[FirstEvent].EventType;
				FirstEvent++;
				if (FirstEvent == MAX_EVENTS)
					FirstEvent = 0;
			}
			NumEvents -= tmpevents;
			tmpevents = 0;
			XmitByteCount = b;
		break;
		default:
			XmitBuffer[0] = 0;
			XmitByteCount = 1;	//we need to set xmit to at least 1 byte, but its 0 so its OK
		break;
	}
	BkPln_I2C_SlaveSendBlock(BkPlnPtr, XmitBuffer, XmitByteCount);
}

void BkPln_I2C_OnBusStopDetected(LDD_TUserData *UserDataPtr)
{
	uint_8 a;

	BkPln_I2C_SlaveCancelReceptionBlock(BkPlnPtr);
	BkPln_I2C_SlaveCancelTransmissionBlock(BkPlnPtr);
	if (SlaveStatus == SLAVE_RX){
		switch(AddressByte){
			case SFP_PROV_ADDRESS:
				if (RcvByteCount == 17){
					TmpNetTXDisableState = RcvBuffer[1];
					TmpNetTXRateState = RcvBuffer[2];
					TmpNetTXPowerState = RcvBuffer[3];
					TmpNetRXRateState = RcvBuffer[4];
					TmpCustTXDisableState = RcvBuffer[5];
					TmpCustTXRateState = RcvBuffer[6];
					TmpCustTXPowerState = RcvBuffer[7];
					TmpCustRXRateState = RcvBuffer[8];
					for (a = 0; a < 4; a++){
						tmpflags[a] = RcvBuffer[a + 8] << 8;
						tmpflags[a] = RcvBuffer[a + 9];
					}
					if (TmpNetTXDisableState != *NetTXPowerState && (TmpNetTXDisableState == ENABLE_TX || TmpNetTXDisableState == DISABLE_TX)){
						_int_disable();
						*NetTXPowerState = TmpNetTXDisableState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_NET_P_SFP] &= ~CHANGE_DISABLE;
						tmpflags[SOURCE_NET_S_SFP] &= ~CHANGE_DISABLE;
					}
					if (TmpCustTXDisableState != *CustTXPowerState && (TmpCustTXDisableState == ENABLE_TX || TmpCustTXDisableState == DISABLE_TX)){
						_int_disable();
						*CustTXPowerState = TmpCustTXDisableState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_CUST_P_SFP] &= ~CHANGE_DISABLE;
						tmpflags[SOURCE_CUST_S_SFP] &= ~CHANGE_DISABLE;
					}
					if (TmpNetTXRateState != *NetTXRateState && (TmpNetTXRateState == LOW_RATE_TX || TmpNetTXRateState == HIGH_RATE_TX)){
						_int_disable();
						*NetTXRateState = TmpNetTXRateState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_NET_P_SFP] &= ~CHANGE_TX_RATE;
						tmpflags[SOURCE_NET_S_SFP] &= ~CHANGE_TX_RATE;
					}
					if (TmpCustTXRateState != *CustTXRateState && (TmpCustTXRateState == LOW_RATE_TX || TmpCustTXRateState == HIGH_RATE_TX)){
						_int_disable();
						*CustTXRateState = TmpCustTXRateState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_CUST_P_SFP] &= ~CHANGE_TX_RATE;
						tmpflags[SOURCE_CUST_S_SFP] &= ~CHANGE_TX_RATE;
					}
					if (TmpNetTXPowerState != *NetTXPowerState && (TmpNetTXPowerState == LOW_POWER || TmpNetTXPowerState == HIGH_POWER)){
						_int_disable();
						*NetTXPowerState = TmpNetTXPowerState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_NET_P_SFP] &= ~CHANGE_POWER;
						tmpflags[SOURCE_NET_S_SFP] &= ~CHANGE_POWER;
					}
					if (TmpCustTXPowerState != *CustTXPowerState && (TmpCustTXPowerState == LOW_POWER || TmpCustTXPowerState == HIGH_POWER)){
						_int_disable();
						*CustTXPowerState = TmpCustTXPowerState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_CUST_P_SFP] &= ~CHANGE_POWER;
						tmpflags[SOURCE_CUST_S_SFP] &= ~CHANGE_POWER;
					}
					if (TmpNetRXRateState != *NetRXRateState && (TmpNetRXRateState == LOW_RATE_RX || TmpNetRXRateState == HIGH_RATE_RX)){
						_int_disable();
						*NetRXRateState = TmpNetRXRateState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_NET_P_SFP] &= ~CHANGE_RX_RATE;
						tmpflags[SOURCE_NET_S_SFP] &= ~CHANGE_RX_RATE;
					}
					if (TmpCustRXRateState != *CustRXRateState && (TmpCustRXRateState == LOW_RATE_RX || TmpCustRXRateState == HIGH_RATE_RX)){
						_int_disable();
						*CustRXRateState = TmpCustRXRateState;
			        	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
			        	_int_enable();
					}
					else{
						tmpflags[SOURCE_CUST_P_SFP] &= ~CHANGE_RX_RATE;
						tmpflags[SOURCE_CUST_S_SFP] &= ~CHANGE_RX_RATE;
					}
				}
			break;
		}
	}
	SlaveStatus = SLAVE_IDLE;
}

#ifdef USE_BKPLN_TASK
void BackplaneTask(uint32_t value){
	SlaveStatus = SLAVE_IDLE;
	RcvByteCount = 0;
	StopBit = 0;
	AddressByte = 0;
	BkPlnPtr = BkPln_I2C_Init(NULL);
	I2C0_A1 = I2C_A1_AD(UnitAddress);
	while (1){
		_time_delay(10);
	}
}
#else
void BackplaneInit(){
	SlaveStatus = SLAVE_IDLE;
	RcvByteCount = 0;
	StopBit = 0;
	AddressByte = 0;
	BkPlnPtr = BkPln_I2C_Init(NULL);
	I2C0_A1 = I2C_A1_AD(UnitAddress);
}

#endif

