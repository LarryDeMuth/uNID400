/*
 * ISR.c
 *
 *  Created on: Nov 28, 2014
 *      Author: Larry
 */
#include <mqx.h>
#include "extern.h"
#include "config.h"

extern void LogEvent(uint8_t event, uint8_t source);
static void UpdateLEDs();

#ifdef HAS_EXT_RTC
void RTC_Int_OnInterrupt(pointer user_isr_ptr){
	if (TIMER_SEM.VALID && TIMER_SEM.TD_QUEUE.SIZE == 1)		//check this first, it may (but shouldn't) already be posted
		_lwsem_post(&TIMER_SEM);								//tell main, 1 second expired
	ReadSFPS = 1;												//time to read the SFP's
	UpdateTime = 1;												//for craft screens only
}

void ExtRTC_SPI_OnBlockReceived(LDD_TUserData *UserDataPtr){
	RTCSPIReceivedFlag = TRUE;
	RTCSPIRcvBusy = 0;
}

void ExtRTC_SPI_OnBlockSent(LDD_TUserData *UserDataPtr){
	RTCSPITransmittedFlag = TRUE;
	RTCSPIXmtBusy = 0;
}
#endif

void Temp_SPI_OnBlockSent(LDD_TUserData *UserDataPtr)
{
	TempSPITransmittedFlag = TRUE;
	TempSPIXmtBusy = 0;
}

void Temp_SPI_OnBlockReceived(LDD_TUserData *UserDataPtr)
{
	TempSPIReceivedFlag = TRUE;
	TempSPIRcvBusy = 0;
}

void SFPs_I2C_OnMasterBlockSent(LDD_TUserData *UserDataPtr){
	SFPDataTransmittedFlg = TRUE;								//if send complete, set complete flag
	SFPXmitBusy = 0;											//clear the busy count
}

void SFPs_I2C_OnMasterBlockReceived(LDD_TUserData *UserDataPtr){
	SFPDataReceivedFlg = TRUE;
	SFPRcvBusy = 0;
}

void Mux_I2C_OnMasterBlockSent(LDD_TUserData *UserDataPtr){
	MuxDataTransmittedFlg = TRUE;								//if send complete, set complete flag
	MuxXmitBusy = 0;											//clear the busy count
}

void Mux_I2C_OnMasterBlockReceived(LDD_TUserData *UserDataPtr){
	MuxDataReceivedFlg = TRUE;
	MuxRcvBusy = 0;
}

void TU1_OnCounterRestart(pointer user_isr_ptr){	//.01 milisecond interrupt
	uint32_t a;

	if (!SFPDataTransmittedFlg){					//if transmitting...
		if (SFPXmitBusy){							//if timeout started
			SFPXmitBusy--;							//wait for timeout
			if (SFPXmitBusy == 0){					//if timeout
				SFPDataTransmittedFlg = TRUE;		//set complete flag
				SFPError = TRUE;					//and error flag
			}
		}
		else										//if not...
			SFPXmitBusy = TRANSMIT_TIMEOUT;			//start it
	}
	if (!SFPDataReceivedFlg){
		if (SFPRcvBusy){
			SFPRcvBusy--;
			if (SFPRcvBusy == 0){
				SFPDataReceivedFlg = TRUE;
				SFPError = TRUE;
			}
		}
		else
			SFPRcvBusy = I2C_RECEIVE_TIMEOUT;
	}

	if (!MuxDataTransmittedFlg){					//if transmitting...
		if (MuxXmitBusy){							//if timeout started
			MuxXmitBusy--;							//wait for timeout
			if (MuxXmitBusy == 0)					//if timeout
				MuxDataTransmittedFlg = TRUE;		//set complete flag
		}
		else										//if not...
			MuxXmitBusy = TRANSMIT_TIMEOUT;			//start it
	}
	if (!MuxDataReceivedFlg){
		if (MuxRcvBusy){
			MuxRcvBusy--;
			if (MuxRcvBusy == 0)
				MuxDataReceivedFlg = TRUE;
		}
		else
			MuxRcvBusy = I2C_RECEIVE_TIMEOUT;
	}

	if (!TempSPITransmittedFlag){					//if transmitting...
		if (TempSPIXmtBusy){							//if timeout started
			TempSPIXmtBusy--;							//wait for timeout
			if (TempSPIXmtBusy == 0)					//if timeout
				TempSPITransmittedFlag = TRUE;		//set complete flag
		}
		else										//if not...
			TempSPIXmtBusy = TRANSMIT_TIMEOUT;			//start it
	}
	if (!TempSPIReceivedFlag){
		if (TempSPIRcvBusy){
			TempSPIRcvBusy--;
			if (TempSPIRcvBusy == 0)
				TempSPIReceivedFlag = TRUE;
		}
		else
			TempSPIRcvBusy = I2C_RECEIVE_TIMEOUT;
	}
#ifdef HAS_EXT_RTC
	if (!RTCSPITransmittedFlag){
		if (RTCSPIXmtBusy){
			RTCSPIXmtBusy--;
			if (RTCSPIXmtBusy == 0)
				RTCSPITransmittedFlag = TRUE;
		}
		else
			RTCSPIXmtBusy = 4;
	}

	if (!RTCSPIReceivedFlag){
		if (RTCSPIRcvBusy){
			RTCSPIRcvBusy--;
			if (RTCSPIRcvBusy == 0)
				RTCSPIReceivedFlag = TRUE;
		}
		else
			RTCSPIRcvBusy = 4;
	}
#endif
#ifdef HAS_CRAFT_FW_UPDATE
	if (ProgramTimeout){
		ProgramTimeout--;
		if (ProgramTimeout == 0){
			UARTFWUpdate = false;
			RestartUART = true;
		}
	}
#endif
	a = PortD_GetFieldValue(PortD_DeviceData, ModulePresent);
	NoSFP[SOURCE_NET_P_SFP] = a & NET_P_PIN;
	NoSFP[SOURCE_NET_S_SFP] = a & NET_S_PIN;
	NoSFP[SOURCE_CUST_P_SFP] = a & CUST_P_PIN;
	NoSFP[SOURCE_CUST_S_SFP] = a & CUST_S_PIN;
	a = PortC_GetFieldValue(PortC_DeviceData, Loss);
	ILOS[SOURCE_NET_P_SFP] = !(a & NET_P_PIN);
	ILOS[SOURCE_NET_S_SFP] = !(a & NET_S_PIN);
	ILOS[SOURCE_CUST_P_SFP] = !(a & CUST_P_PIN);
	ILOS[SOURCE_CUST_S_SFP] = !(a & CUST_S_PIN);
	a = PortC_GetFieldValue(PortC_DeviceData, NP_Pins);
	if (!NoSFP[SOURCE_NET_P_SFP] && (SFPStatus[SOURCE_NET_P_SFP].Options & TX_FAULT)){
		Fault[SOURCE_NET_P_SFP] = a & FAULT_PIN;
		if (SFPStatus[SOURCE_NET_P_SFP].Options & (TX_SOFT_DISABLE | TX_SOFT_FAULT) != (TX_SOFT_DISABLE | TX_SOFT_FAULT)){
			if (Fault[SOURCE_NET_P_SFP] && !(SFPStatus[SOURCE_NET_P_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){					//if we havent logged it yet
				LogEvent(EVENT_TX_FAULT, SOURCE_NET_P_SFP);		//log it
				SFPStatus[SOURCE_NET_P_SFP].SFPFlags |= SFP_EVENT_TX_FAULT;
			}
			else if (!Fault[SOURCE_NET_P_SFP] && (SFPStatus[SOURCE_NET_P_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){	//if no fault and we logged a fault
				LogEvent(EVENT_TX_FAULT_CLEAR, SOURCE_NET_P_SFP);				//log cleared
				SFPStatus[SOURCE_NET_P_SFP].SFPFlags &= ~SFP_EVENT_TX_FAULT;
			}
		}
	}
	else
		Fault[SOURCE_NET_P_SFP] = false;

	a = PortC_GetFieldValue(PortC_DeviceData, NS_Pins);
	if (!NoSFP[SOURCE_NET_S_SFP] && (SFPStatus[SOURCE_NET_S_SFP].Options & TX_FAULT)){
		Fault[SOURCE_NET_S_SFP] = a & FAULT_PIN;
		if (SFPStatus[SOURCE_NET_S_SFP].Options & (TX_SOFT_DISABLE | TX_SOFT_FAULT) != (TX_SOFT_DISABLE | TX_SOFT_FAULT)){
			if (Fault[SOURCE_NET_S_SFP] && !(SFPStatus[SOURCE_NET_S_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){					//if we havent logged it yet
				LogEvent(EVENT_TX_FAULT, SOURCE_NET_S_SFP);		//log it
				SFPStatus[SOURCE_NET_S_SFP].SFPFlags |= SFP_EVENT_TX_FAULT;
			}
			else if (!Fault[SOURCE_NET_S_SFP] && (SFPStatus[SOURCE_NET_S_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){	//if no fault and we logged a fault
				LogEvent(EVENT_TX_FAULT_CLEAR, SOURCE_NET_S_SFP);				//log cleared
				SFPStatus[SOURCE_NET_S_SFP].SFPFlags &= ~SFP_EVENT_TX_FAULT;
			}
		}
	}
	else
		Fault[SOURCE_NET_S_SFP] = false;

	a = PortC_GetFieldValue(PortC_DeviceData, CP_Pins);
	if (!NoSFP[SOURCE_CUST_P_SFP] && (SFPStatus[SOURCE_CUST_P_SFP].Options & TX_FAULT)){
		Fault[SOURCE_CUST_P_SFP] = a & FAULT_PIN;
		if (SFPStatus[SOURCE_CUST_P_SFP].Options & (TX_SOFT_DISABLE | TX_SOFT_FAULT) != (TX_SOFT_DISABLE | TX_SOFT_FAULT)){
			if (Fault[SOURCE_CUST_P_SFP] && !(SFPStatus[SOURCE_CUST_P_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){					//if we havent logged it yet
				LogEvent(EVENT_TX_FAULT, SOURCE_CUST_P_SFP);		//log it
				SFPStatus[SOURCE_CUST_P_SFP].SFPFlags |= SFP_EVENT_TX_FAULT;
			}
			else if (!Fault[SOURCE_CUST_P_SFP] && (SFPStatus[SOURCE_CUST_P_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){	//if no fault and we logged a fault
				LogEvent(EVENT_TX_FAULT_CLEAR, SOURCE_CUST_P_SFP);				//log cleared
				SFPStatus[SOURCE_CUST_P_SFP].SFPFlags &= ~SFP_EVENT_TX_FAULT;
			}
		}
	}
	else
		Fault[SOURCE_CUST_P_SFP] = false;

	a = PortC_GetFieldValue(PortC_DeviceData, CS_Pins);
	if (!NoSFP[SOURCE_CUST_S_SFP] && (SFPStatus[SOURCE_CUST_S_SFP].Options & TX_FAULT)){
		Fault[SOURCE_CUST_S_SFP] = a & FAULT_PIN;
		if (SFPStatus[SOURCE_CUST_S_SFP].Options & (TX_SOFT_DISABLE | TX_SOFT_FAULT) != (TX_SOFT_DISABLE | TX_SOFT_FAULT)){
			if (Fault[SOURCE_CUST_S_SFP] && !(SFPStatus[SOURCE_CUST_S_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){					//if we havent logged it yet
				LogEvent(EVENT_TX_FAULT, SOURCE_CUST_S_SFP);		//log it
				SFPStatus[SOURCE_CUST_S_SFP].SFPFlags |= SFP_EVENT_TX_FAULT;
			}
			else if (!Fault[SOURCE_CUST_S_SFP] && (SFPStatus[SOURCE_CUST_S_SFP].SFPFlags & SFP_EVENT_TX_FAULT)){	//if no fault and we logged a fault
				LogEvent(EVENT_TX_FAULT_CLEAR, SOURCE_CUST_S_SFP);				//log cleared
				SFPStatus[SOURCE_CUST_S_SFP].SFPFlags &= ~SFP_EVENT_TX_FAULT;
			}
		}
	}
	else
		Fault[SOURCE_CUST_S_SFP] = false;

	TenMilisecondCount++;
	if (TenMilisecondCount == 20){				//if 200ms
		TenMilisecondCount = 0;
		UpdateLEDs();
		HundredMilisecondCount++;
		if (HundredMilisecondCount == 5){			//if one second
			HundredMilisecondCount = 0;
#ifndef HAS_EXT_RTC
			if (TIMER_SEM.VALID && TIMER_SEM.TD_QUEUE.SIZE == 1)		//check this first, it may (but shouldn't) already be posted
				_lwsem_post(&TIMER_SEM);								//tell main, 1 second expired
			ReadSFPS = 1;												//time to read the SFP's
#endif
#ifdef HAS_CRAFT
#ifdef HAS_CRAFT_FW_UPDATE
			if (!UARTFWUpdate){
#endif
				if (ScreenState == SEEKING){			//if looking for craft connection
					TerminalQuery = 1;				//query the craft port every second
				}
				else{
					CraftPortTimer++;				//if we are connected to the craft
					if (CraftPortTimer == 5 || CraftPortTimer == 10 || CraftPortTimer == 15){	//query every 5 seconds
						if (CraftPortTimer == 15){				//if no activity after 15 seconds, then no craft connection
							ScreenState = SEEKING;				//go back to looking for it
							CraftPortTimer = 0;
						}
						TerminalQuery = 1;
					}
				}
#ifdef HAS_CRAFT_FW_UPDATE
			}
#endif
#endif
		}
	}
}

static void UpdateLEDs(){
	uint32_t LEDReg = 0xffffffff;

	if (NoSFP[SOURCE_NET_P_SFP] || ILOS[SOURCE_NET_P_SFP])
		LEDReg &= ~NET_P_LOS;

	if (LEDState[NET_P_PROB] == LED_ON)		//SFP net alarm
		LEDReg &= ~NET_P_PROB;
	else if (LEDState[NET_P_PROB] == LED_BLINK && FlashLED < 2)	//SFP net warning
		LEDReg &= ~NET_P_PROB;

	if (LEDState[NET_P_SPEED] == LED_ON)		//100M speed
		LEDReg &= ~NET_P_SPEED;
	else if (LEDState[NET_P_SPEED] == LED_BLINK && FlashLED < 2)	//10G speed
		LEDReg &= ~NET_P_SPEED;
	else if (LEDState[NET_P_SPEED] == LED_PULSE && PulseLED == 0)	//1G speed
		LEDReg &= ~NET_P_SPEED;

	if (LEDState[NET_P_OP] == LED_ON)
		LEDReg &= ~NET_P_OP;
	else if (LEDState[NET_P_OP] == LED_BLINK && FlashLED < 2)
		LEDReg &= ~NET_P_OP;

	if (NoSFP[SOURCE_NET_S_SFP] || ILOS[SOURCE_NET_S_SFP])
		LEDReg &= ~NET_S_LOS;

	if (LEDState[NET_S_PROB] == LED_ON)		//SFP net alarm
		LEDReg &= ~NET_S_PROB;
	else if (LEDState[NET_S_PROB] == LED_BLINK && FlashLED < 2)	//SFP net warning
		LEDReg &= ~NET_S_PROB;

	if (LEDState[NET_S_SPEED] == LED_ON)		//100M speed
		LEDReg &= ~NET_S_SPEED;
	else if (LEDState[NET_S_SPEED] == LED_BLINK && FlashLED < 2)	//10G speed
		LEDReg &= ~NET_S_SPEED;
	else if (LEDState[NET_S_SPEED] == LED_PULSE && PulseLED == 0)	//1G speed
		LEDReg &= ~NET_S_SPEED;

	if (LEDState[NET_S_OP] == LED_ON)
		LEDReg &= ~NET_S_OP;
	else if (LEDState[NET_S_OP] == LED_BLINK && FlashLED < 2)
		LEDReg &= ~NET_S_OP;

	if (NoSFP[SOURCE_CUST_P_SFP] || ILOS[SOURCE_CUST_P_SFP])
		LEDReg &= ~CUST_P_LOS;

	if (LEDState[CUST_P_PROB] == LED_ON)		//SFP cust alarm
		LEDReg &= ~CUST_P_PROB;
	else if (LEDState[CUST_P_PROB] == LED_BLINK && FlashLED < 2)	//SFP cust warning
		LEDReg &= ~CUST_P_PROB;

	if (LEDState[CUST_P_SPEED] == LED_ON)		//100M speed
		LEDReg &= ~CUST_P_SPEED;
	else if (LEDState[CUST_P_SPEED] == LED_BLINK && FlashLED < 2)	//10G speed
		LEDReg &= ~CUST_P_SPEED;
	else if (LEDState[CUST_P_SPEED] == LED_PULSE && PulseLED == 0)	//1G speed
		LEDReg &= ~CUST_P_SPEED;

	if (LEDState[CUST_P_OP] == LED_ON)
		LEDReg &= ~CUST_P_OP;
	else if (LEDState[CUST_P_OP] == LED_BLINK && FlashLED < 2)
		LEDReg &= ~CUST_P_OP;

	if (NoSFP[SOURCE_CUST_S_SFP] || ILOS[SOURCE_CUST_S_SFP])
		LEDReg &= ~CUST_S_LOS;

	if (LEDState[CUST_S_PROB] == LED_ON)		//SFP cust alarm
		LEDReg &= ~CUST_S_PROB;
	else if (LEDState[CUST_S_PROB] == LED_BLINK && FlashLED < 2)	//SFP cust warning
		LEDReg &= ~CUST_S_PROB;

	if (LEDState[CUST_S_SPEED] == LED_ON)		//100M speed
		LEDReg &= ~CUST_S_SPEED;
	else if (LEDState[CUST_S_SPEED] == LED_BLINK && FlashLED < 2)	//10G speed
		LEDReg &= ~CUST_S_SPEED;
	else if (LEDState[CUST_S_SPEED] == LED_PULSE && PulseLED == 0)	//1G speed
		LEDReg &= ~CUST_S_SPEED;

	if (LEDState[CUST_S_OP] == LED_ON)
		LEDReg &= ~CUST_S_OP;
	else if (LEDState[CUST_S_OP] == LED_BLINK && FlashLED < 2)
		LEDReg &= ~CUST_S_OP;

	if (LEDState[PWR_RST] == LED_BLINK && FlashLED < 2)	//when booting..
		LEDReg &= ~PWR_RST;
	else
		LEDReg &= ~PWR_LED;

	if (LEDState[PROT_LED] == LED_ON)
		LEDReg &= ~PROT_LED;

	if (LEDState[MON_LED] == LED_ON)
		LEDReg &= ~MON_LED;

	if (LEDState[NID_MAJOR] == LED_ON)		//NID major alarm
		LEDReg &= ~NID_MAJOR;

	if (LEDState[NID_MINOR] == LED_ON)		//NID minor alarm
		LEDReg &= ~NID_MINOR;

	PortA_SetFieldValue(PortA_DeviceData, LED_Lo, LED_LOW_FIELD(LEDReg));
	PortB_SetFieldValue(PortB_DeviceData, LED_Med, LED_MEDIUM_FIELD(LEDReg));
	PortB_SetFieldValue(PortB_DeviceData, LED_Hi, LED_HIGH_FIELD(LEDReg));

	FlashLED++;
	if (FlashLED == 4)
		FlashLED = 0;
	PulseLED++;
	if (PulseLED == 8)
		PulseLED = 0;

}

