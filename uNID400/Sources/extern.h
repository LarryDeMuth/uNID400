/*
 * extern.h
 *
 *  Created on: Nov 25, 2014
 *      Author: Larry
 */

#ifndef EXTERN_H_
#define EXTERN_H_

#include <mqx.h>
#include <bsp.h>
#include "config.h"

extern bool NoSFP[4], ILOS[4], Fault[4];

extern bool SFPError, PowerDownTime;

extern uint_8 UnitAddress;
extern uint_8 SwitchOptions;

struct SFPDataA0{			//address 0xA0
	uint8_t 	Identifier;
	uint8_t		ExtIdentifier;
	uint8_t		Connector;
	uint8_t		Tranceiver[8];
	uint8_t		Encoding;
	uint8_t		BRNominal;
	uint8_t		RateIdentifier;
	uint8_t		LengthSMFkm;
	uint8_t		LengthSMF;
	uint8_t		Length50um;
	uint8_t		Length62_5um;
	uint8_t		LengthCable;
	uint8_t		LengthOM3;
	uint8_t		VendorName[16];
	uint8_t		TranceiverCode;
	uint8_t		VendorOUI[3];
	uint8_t		VendorPN[16];
	uint8_t		VendorRev[4];
	uint8_t		WaveLength[2];
	uint8_t		Unallocated;
	uint8_t		CCBase;
	uint8_t		Options[2];
	uint8_t		BRMax;
	uint8_t		BRMin;
	uint8_t		VendorSN[16];
	struct datecode{
		uint8_t		Year[2];
		uint8_t		Month[2];
		uint8_t		Day[2];
		uint8_t		LotCode[2];
	}DateCode;
	uint8_t		DiagMonType;
	uint8_t		EnhancedOptions;
	uint8_t		SFF_8472Compilance;
	uint8_t		CC_Ext;
};
extern struct SFPDataA0 SFPDataA0[4];

struct SFPDataA2{			//address 0xA2
	uint8_t	TempHighAlarm[2];
	uint8_t	TempLowAlarm[2];
	uint8_t	TempHighWarning[2];
	uint8_t	TempLowWarning[2];
	uint8_t	VoltageHighAlarm[2];
	uint8_t	VoltageLowAlarm[2];
	uint8_t	VoltageHighWarning[2];
	uint8_t	VoltageLowWarning[2];
	uint8_t	BiasHighAlarm[2];
	uint8_t	BiasLowAlarm[2];
	uint8_t	BiasHighWarning[2];
	uint8_t	BiasLowWarning[2];
	uint8_t	TXPowerHighAlarm[2];
	uint8_t	TXPowerLowAlarm[2];
	uint8_t	TXPowerHighWarning[2];
	uint8_t	TXPowerLowWarning[2];
	uint8_t	RXPowerHighAlarm[2];
	uint8_t	RXPowerLowAlarm[2];
	uint8_t	RXPowerHighWarning[2];
	uint8_t	RXPowerLowWarning[2];
	uint8_t LaserTempHighAlarm[2];
	uint8_t LaserTempLowAlarm[2];
	uint8_t LaserTempHighWarning[2];
	uint8_t LaserTempLowWarning[2];
	uint8_t TECCurrentHighAlarm[2];
	uint8_t	TECCurrentLowAlarm[2];
	uint8_t TECCurrentHighWarning[2];
	uint8_t TECCurrentLowWarning[2];
	uint8_t	RxPWR4[4];
	uint8_t	RxPWR3[4];
	uint8_t	RxPWR2[4];
	uint8_t	RxPWR1[4];
	uint8_t	RxPWR0[4];
	uint8_t TxISlope[2];
	uint8_t TxIOffset[2];
	uint8_t	TxPWRSlope[2];
	uint8_t	TxPWROffset[2];
	uint8_t	TSlope[2];
	uint8_t	TOffset[2];
	uint8_t	VSlope[2];
	uint8_t	VOffset[2];
	uint8_t	Reserved[3];
	uint8_t	Checksum;
	int8_t	IntTemp[2];
	uint8_t	IntSupplyV[2];
	uint8_t	TxBiasCurrent[2];
	uint8_t	TxOutputPower[2];
	uint8_t	RxInputPower[2];
	int8_t 	LaserTemp_WL[2];
	int8_t 	TECurrent[2];
	uint8_t	StatusControl;
	uint8_t	Reserved3;
	uint8_t	AlarmFlags[2];
	uint8_t	InputEqualizationControl;
	uint8_t OutputEmphasisControl;
	uint8_t	WarningFlags[2];
	uint8_t	ExtStatusControl[2];
};
extern struct SFPDataA2 SFPDataA2[4];

struct sfpdata{
	uint16_t SFPFlags;			//see config.h for bits
	uint16_t EventAlarmFlags;
	uint16_t EventWarnFlags;
	uint8_t SFPType;
	uint8_t ReadFail;
	uint8_t Options;
	uint8_t FaultCount;
	uint8_t SpeedH;
	uint8_t SpeedL;
};
extern struct sfpdata SFPStatus[4];

struct actualvalues{
	float IntTemp;
	float Voltage;
	float Bias;
	float TXPower;
	float RXPower;
	float LaserTemp;
	float TECCurrent;
	uint16_t AlarmFlags;
	uint16_t WarnFlags;
};
extern struct actualvalues ActualValues[4];

#ifdef HAS_INTERNAL_ELOG
struct eventlog{
	uint32_t TimeStamp;
	uint8_t	EventType;
	uint8_t	Source;
};
struct eventlog WorkingLog;

#else
struct eventlog{
	uint8_t	EventType;
	uint8_t	Source;
};
struct eventlog WorkingLog[MAX_EVENTS];
#endif
extern int16_t LastEvent, FirstEvent;
extern uint16_t NumEvents;

extern bool EventMismatch;
extern bool UpdateProvision;
extern uint8_t TmpNetTXDisableState, TmpCustTXDisableState;
extern uint8_t TmpNetTXRateState, TmpCustTXRateState;
extern uint8_t TmpNetTXPowerState, TmpCustTXPowerState;
extern uint8_t TmpNetRXRateState, TmpCustRXRateState;
extern uint_16 tmpflags[4];

#ifdef HAS_MEMO_PAD
typedef struct memolog{//NOTE: Each memo is 1 sector long. Thats why 124 bytes for the memo and 4 for the timestamp
	uint32_t TimeStamp;
	char Memo[124];
}memolog;
extern uint8_t NumMemos;
extern int8_t LastMemo, FirstMemo;
extern struct memolog WorkingMemo;
#endif

extern uint_16 ProgramTimeout;

extern uint8_t TenMilisecondCount, HundredMilisecondCount, OneSecondCount;
extern uint8_t PulseLED, FlashLED;

extern ledstate LEDState[FLASHING_LED_END];

extern MQX_FILE_PTR flash_file;
extern LWSEM_STRUCT TIMER_SEM;

extern bool DefaultTime, UpdateScreen, ReadSFPS, UpdateTime;

extern bool DefaultMiscSettings, DefaultSettings;
extern uint8_t SelectedSFP;

extern char RcvdProductString[33];
extern uint8_t TFTPUpdateState;
extern uint32_t TFTPError;
extern uint32_t SRECLength;
extern bool Reboot;

extern volatile bool SFPDataReceivedFlg;
extern volatile bool SFPDataTransmittedFlg;
extern volatile bool MuxDataReceivedFlg;
extern volatile bool MuxDataTransmittedFlg;
extern char SFPRcvBusy, SFPXmitBusy;
extern char MuxRcvBusy, MuxXmitBusy;
extern LDD_TDeviceData *SFPPtr, *MuxPtr;
extern LDD_TDeviceData *BkPlnPtr;

extern LDD_TDeviceData *Timer1Ptr;
extern volatile bool RTCSPIReceivedFlag;
extern volatile bool RTCSPITransmittedFlag;
extern volatile bool TempSPIReceivedFlag;
extern volatile bool TempSPITransmittedFlag;
extern char RTCSPIRcvBusy, RTCSPIXmtBusy, TempSPIRcvBusy, TempSPIXmtBusy;
extern LDD_TDeviceData *RTCSPIPtr, *TempSPIPtr;
extern LDD_RTC_TTime RTC_TTime, NewTime, EventTime;

#endif /* EXTERN_H_ */
