#include <mqx.h>
#include <string.h>
#include "extern.h"
#include "config.h"

extern void LogEvent(uint_8 event, uint_8 source);

void DefaultSFPData(uint_8 side);
void DefaultSFPDataA2(uint_8 side);
void GetA2Values(uint_8 side);
static float GetFloat(uint_8 *byte);

void GetA2Values(uint_8 side){
	struct actualvalues *values;
	struct SFPDataA0 *A0Data;
	struct SFPDataA2 *A2Data;
	float tmpvalue, tmp;
	uint_16 tmpalarm, tmpwarn;
	uint_16 *Alarms, *Warnings;

	if (side < SOURCE_NET_P_SFP || side > SOURCE_CUST_S_SFP)
		return;
	values = &ActualValues[side];
	A2Data = &SFPDataA2[side];
	A0Data = &SFPDataA0[side];
	Alarms = &SFPStatus[side].EventAlarmFlags;
	Warnings = &SFPStatus[side].EventWarnFlags;

	tmpvalue = A2Data->IntTemp[0];
	tmpvalue += (uint_8)A2Data->IntTemp[1] / 256.0;
	if (A0Data->DiagMonType & EXTERNAL_CALIBRATION)
		tmpvalue = (tmpvalue * (float)(A2Data->TSlope[0] + (A2Data->TSlope[1] / 256.0))) + (int_16)(A2Data->TOffset[0] << 8 | A2Data->TOffset[1]);
	values->IntTemp = tmpvalue;
	tmpvalue = (A2Data->IntSupplyV[0] << 8 | A2Data->IntSupplyV[1]) / 10000.0;
	if (A0Data->DiagMonType & EXTERNAL_CALIBRATION)
		tmpvalue = (tmpvalue * (float)(A2Data->VSlope[0] + (A2Data->VSlope[1] / 256.0))) + (int_16)(A2Data->VOffset[0] << 8 | A2Data->VOffset[1]);
	values->Voltage = tmpvalue;
	tmpvalue = (A2Data->TxBiasCurrent[0] << 8 | A2Data->TxBiasCurrent[1]) * .002;
	if (A0Data->DiagMonType & EXTERNAL_CALIBRATION)
		tmpvalue = (tmpvalue * (float)(A2Data->TxISlope[0] + (A2Data->TxISlope[1] / 256.0))) + (int_16)(A2Data->TxIOffset[0] << 8 | A2Data->TxIOffset[1]);
	values->Bias = tmpvalue;
	tmpvalue = (A2Data->TxOutputPower[0] << 8 | A2Data->TxOutputPower[1]) / 10000.0;
	if (A0Data->DiagMonType & EXTERNAL_CALIBRATION)
		tmpvalue = (tmpvalue * (float)(A2Data->TxPWRSlope[0] + (A2Data->TxPWRSlope[1] / 256.0))) + (int_16)(A2Data->TxPWROffset[0] << 8 | A2Data->TxPWROffset[1]);
	values->TXPower = tmpvalue;
	tmpvalue = (A2Data->RxInputPower[0] << 8 | A2Data->RxInputPower[1]) / 10000.0;
	if (A0Data->DiagMonType & EXTERNAL_CALIBRATION){
		tmp = GetFloat(A2Data->RxPWR0);
		tmp += (GetFloat(A2Data->RxPWR1) * tmpvalue);
		tmp += (GetFloat(A2Data->RxPWR2) * tmpvalue * tmpvalue);
		tmp += (GetFloat(A2Data->RxPWR3) * tmpvalue * tmpvalue * tmpvalue);
		tmp += (GetFloat(A2Data->RxPWR4) * tmpvalue * tmpvalue * tmpvalue * tmpvalue);
		tmpvalue = tmp;
	}
	values->RXPower = tmpvalue;
	if (A0Data->Identifier == DWDM_SFP && (A0Data->DiagMonType & INTERNAL_CALIBRATION)){
		tmpvalue = A2Data->LaserTemp_WL[0];
		tmpvalue += (uint_8)A2Data->LaserTemp_WL[1] / 256.0;
		values->LaserTemp = tmpvalue;

		values->TECCurrent = (((A2Data->TECurrent[0] << 8) | A2Data->TECurrent[1]) / 10);
	}
	if (A0Data->EnhancedOptions & FLAGS_IMPLEMENTED){
		//Per SFF 8472 verify asserted after 100ms before declaring (values is last read, data is current read)
		tmpalarm = (A2Data->AlarmFlags[0] << 8 | A2Data->AlarmFlags[1]) & values->AlarmFlags;
		tmpwarn =  (A2Data->WarningFlags[0] << 8 | A2Data->WarningFlags[1]) & values->WarnFlags;
		//its important that alarm flags are checked first, then warns. Should NEVER have high AND low at the same time...
		if(tmpalarm & TEMP_HIGH){						//if a high alarm...
			if (!(*Alarms & TEMP_HIGH)){				//if not logged...
				LogEvent(EVENT_TEMP_HI_ALARM, side);	//log it
				*Alarms |= TEMP_HIGH;					//flag as logged
			}
		}
		else if(tmpwarn & TEMP_HIGH){					//if high warn...
			if (!(*Warnings & TEMP_HIGH)){				//if not logged
				LogEvent(EVENT_TEMP_HI_WARN, side);	//log it
				*Warnings |= TEMP_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm & TEMP_LOW){					//if a low alarm...
			if (!(*Alarms & TEMP_LOW)){					//if not logged...
				LogEvent(EVENT_TEMP_LO_ALARM, side);	//log it
				*Alarms |= TEMP_LOW;					//flag as logged
			}
		}
		else if(tmpwarn & TEMP_LOW){					//if low warn...
			if (!(*Warnings & TEMP_LOW)){				//if not logged
				LogEvent(EVENT_TEMP_LO_WARN, side);	//log it
				*Warnings |= TEMP_LOW;					//flag as logged
			}
		}
		else{											//if no alarms or warns for temp
														//if at least 1 was logged
			if (((*Alarms & (TEMP_HIGH | TEMP_LOW)) | (*Warnings & (TEMP_HIGH | TEMP_LOW))) != 0){
				LogEvent(EVENT_TEMP_OK, side);		//log it
				*Alarms &= ~(TEMP_HIGH | TEMP_LOW);		//clear logged flags
				*Warnings &= ~(TEMP_HIGH | TEMP_LOW);
			}
		}
		if(tmpalarm & VCC_HIGH){
			if (!(*Alarms & VCC_HIGH)){
				LogEvent(EVENT_VOLT_HI_ALARM, side);
				*Alarms |= VCC_HIGH;
			}
		}
		else if(tmpwarn & VCC_HIGH){
			if (!(*Warnings & VCC_HIGH)){
				LogEvent(EVENT_VOLT_HI_WARN, side);
				*Warnings |= VCC_HIGH;
			}
		}
		else if(tmpalarm & VCC_LOW){
			if (!(*Alarms & VCC_LOW)){
				LogEvent(EVENT_VOLT_LO_ALARM, side);
				*Alarms |= VCC_LOW;
			}
		}
		else if(tmpwarn & VCC_LOW){
			if (!(*Warnings & VCC_LOW)){
				LogEvent(EVENT_VOLT_LO_WARN, side);
				*Warnings |= VCC_LOW;
			}
		}
		else{
			if (((*Alarms & (VCC_HIGH | VCC_LOW)) | (*Warnings & (VCC_HIGH | VCC_LOW))) != 0){
				LogEvent(EVENT_VOLT_OK, side);
				*Alarms &= ~(VCC_HIGH | VCC_LOW);
				*Warnings &= ~(VCC_HIGH | VCC_LOW);
			}
		}
		if(tmpalarm & TX_BIAS_HIGH){
			if (!(*Alarms & TX_BIAS_HIGH)){
				LogEvent(EVENT_BIAS_HI_ALARM, side);
				*Alarms |= TX_BIAS_HIGH;
			}
		}
		else if(tmpwarn & TX_BIAS_HIGH){
			if (!(*Warnings & TX_BIAS_HIGH)){
				LogEvent(EVENT_BIAS_HI_WARN, side);
				*Warnings |= TX_BIAS_HIGH;
			}
		}
		else if(tmpalarm & TX_BIAS_LOW){
			if (!(*Alarms & TX_BIAS_LOW)){
				LogEvent(EVENT_BIAS_LO_ALARM, side);
				*Alarms |= TX_BIAS_LOW;
			}
		}
		else if(tmpwarn & TX_BIAS_LOW){
			if (!(*Warnings & TX_BIAS_LOW)){
				LogEvent(EVENT_BIAS_LO_WARN, side);
				*Warnings |= TX_BIAS_LOW;
			}
		}
		else{
			if (((*Alarms & (TX_BIAS_HIGH | TX_BIAS_LOW)) | (*Warnings & (TX_BIAS_HIGH | TX_BIAS_LOW))) != 0){
				LogEvent(EVENT_BIAS_OK, side);
				*Alarms &= ~(TX_BIAS_HIGH | TX_BIAS_LOW);
				*Warnings &= ~(TX_BIAS_HIGH | TX_BIAS_LOW);
			}
		}
		if(tmpalarm & TX_POWER_HIGH){
			if (!(*Alarms & TX_POWER_HIGH)){
				LogEvent(EVENT_TX_PWR_HI_ALARM, side);
				*Alarms |= TX_POWER_HIGH;
			}
		}
		else if(tmpwarn & TX_POWER_HIGH){
			if (!(*Warnings & TX_POWER_HIGH)){
				LogEvent(EVENT_TX_PWR_HI_WARN, side);
				*Warnings |= TX_POWER_HIGH;
			}
		}
		else if(tmpalarm & TX_POWER_LOW){
			if (!(*Alarms & TX_POWER_LOW)){
				LogEvent(EVENT_TX_PWR_LO_ALARM, side);
				*Alarms |= TX_POWER_LOW;
			}
		}
		else if(tmpwarn & TX_POWER_LOW){
			if (!(*Warnings & TX_POWER_LOW)){
				LogEvent(EVENT_TX_PWR_LO_WARN, side);
				*Warnings |= TX_POWER_LOW;
			}
		}
		else{
			if (((*Alarms & (TX_POWER_HIGH | TX_POWER_LOW)) | (*Warnings & (TX_POWER_HIGH | TX_POWER_LOW))) != 0){
				LogEvent(EVENT_TX_PWR_OK, side);
				*Alarms &= ~(TX_POWER_HIGH | TX_POWER_LOW);
				*Warnings &= ~(TX_POWER_HIGH | TX_POWER_LOW);
			}
		}
		if(tmpalarm & RX_POWER_HIGH){
			if (!(*Alarms & RX_POWER_HIGH)){
				LogEvent(EVENT_RX_PWR_HI_ALARM, side);
				*Alarms |= RX_POWER_HIGH;
			}
		}
		else if(tmpwarn & RX_POWER_HIGH){
			if (!(*Warnings & RX_POWER_HIGH)){
				LogEvent(EVENT_RX_PWR_HI_WARN, side);
				*Warnings |= RX_POWER_HIGH;
			}
		}
		else if(tmpalarm & RX_POWER_LOW){
			if (!(*Alarms & RX_POWER_LOW)){
				LogEvent(EVENT_RX_PWR_LO_ALARM, side);
				*Alarms |= RX_POWER_LOW;
			}
		}
		else if(tmpwarn & RX_POWER_LOW){
			if (!(*Warnings & RX_POWER_LOW)){
				LogEvent(EVENT_RX_PWR_LO_WARN, side);
				*Warnings |= RX_POWER_LOW;
			}
		}
		else{
			if (((*Alarms & (RX_POWER_HIGH | RX_POWER_LOW)) | (*Warnings & (RX_POWER_HIGH | RX_POWER_LOW))) != 0){
				LogEvent(EVENT_RX_PWR_OK, side);
				*Alarms &= ~(RX_POWER_HIGH | RX_POWER_LOW);
				*Warnings &= ~(RX_POWER_HIGH | RX_POWER_LOW);
			}
		}
		if (A0Data->Identifier == DWDM_SFP){
			if(tmpalarm & LASER_TEMP_HIGH){
				if (!(*Alarms & LASER_TEMP_HIGH)){
					LogEvent(EVENT_LASER_HI_ALARM, side);
					*Alarms |= LASER_TEMP_HIGH;
				}
			}
			else if(tmpwarn & LASER_TEMP_HIGH){
				if (!(*Warnings & LASER_TEMP_HIGH)){
					LogEvent(EVENT_LASER_HI_WARN, side);
					*Warnings |= LASER_TEMP_HIGH;
				}
			}
			else if(tmpalarm & LASER_TEMP_LOW){
				if (!(*Alarms & LASER_TEMP_LOW)){
					LogEvent(EVENT_LASER_LO_ALARM, side);
					*Alarms |= LASER_TEMP_LOW;
				}
			}
			else if(tmpwarn & LASER_TEMP_LOW){
				if (!(*Warnings & LASER_TEMP_LOW)){
					LogEvent(EVENT_LASER_LO_WARN, side);
					*Warnings |= LASER_TEMP_LOW;
				}
			}
			else{
				if (((*Alarms & (LASER_TEMP_HIGH | LASER_TEMP_LOW)) | (*Warnings & (LASER_TEMP_HIGH | LASER_TEMP_LOW))) != 0){
					LogEvent(EVENT_LASER_OK, side);
					*Alarms &= ~(LASER_TEMP_HIGH | LASER_TEMP_LOW);
					*Warnings &= ~(LASER_TEMP_HIGH | LASER_TEMP_LOW);
				}
			}
			if(tmpalarm & TEC_CURRENT_HIGH){
				if (!(*Alarms & TEC_CURRENT_HIGH)){
					LogEvent(EVENT_TEC_HI_ALARM, side);
					*Alarms |= TEC_CURRENT_HIGH;
				}
			}
			else if(tmpwarn & TEC_CURRENT_HIGH){
				if (!(*Warnings & TEC_CURRENT_HIGH)){
					LogEvent(EVENT_TEC_HI_WARN, side);
					*Warnings |= TEC_CURRENT_HIGH;
				}
			}
			else if(tmpalarm & TEC_CURRENT_LOW){
				if (!(*Alarms & TEC_CURRENT_LOW)){
					LogEvent(EVENT_TEC_LO_ALARM, side);
					*Alarms |= TEC_CURRENT_LOW;
				}
			}
			else if(tmpwarn & LASER_TEMP_LOW){
				if (!(*Warnings & TEC_CURRENT_LOW)){
					LogEvent(EVENT_TEC_LO_WARN, side);
					*Warnings |= TEC_CURRENT_LOW;
				}
			}
			else{
				if (((*Alarms & (TEC_CURRENT_HIGH | TEC_CURRENT_LOW)) | (*Warnings & (TEC_CURRENT_HIGH | TEC_CURRENT_LOW))) != 0){
					LogEvent(EVENT_TEC_OK, side);
					*Alarms &= ~(TEC_CURRENT_HIGH | TEC_CURRENT_LOW);
					*Warnings &= ~(TEC_CURRENT_HIGH | TEC_CURRENT_LOW);
				}
			}
		}
		else{
			tmpalarm = A2Data->AlarmFlags[0] << 8 | A2Data->AlarmFlags[1];
			tmpwarn =  A2Data->WarningFlags[0] << 8 | A2Data->WarningFlags[1];
			tmpalarm &= ~(TEC_CURRENT_HIGH | TEC_CURRENT_LOW | LASER_TEMP_HIGH | LASER_TEMP_LOW);
			tmpwarn &= ~(TEC_CURRENT_HIGH | TEC_CURRENT_LOW | LASER_TEMP_HIGH | LASER_TEMP_LOW);
			A2Data->AlarmFlags[0] = tmpalarm >> 8;
			A2Data->AlarmFlags[1] = tmpalarm;
			A2Data->WarningFlags[0] = tmpwarn >> 8;
			A2Data->WarningFlags[1] = tmpwarn;
		}
		values->AlarmFlags = (A2Data->AlarmFlags[0] << 8 | A2Data->AlarmFlags[1]);
		values->WarnFlags = (A2Data->WarningFlags[0] << 8 | A2Data->WarningFlags[1]);
	}
	else{		//if not flags, compare to thresholds
		tmpalarm = (A2Data->IntTemp[0] << 8) | A2Data->IntTemp[1];
		if((int_16)tmpalarm > (int_16)((A2Data->TempHighAlarm[0] << 8) | A2Data->TempHighAlarm[1])){//if a high alarm...
			if (!(*Alarms & TEMP_HIGH)){				//if not logged...
				LogEvent(EVENT_TEMP_HI_ALARM, side);	//log it
				*Alarms |= TEMP_HIGH;					//flag as logged
			}
		}
		else if((int_16)tmpalarm > (int_16)((A2Data->TempHighWarning[0] << 8) | A2Data->TempHighWarning[1])){//if high warn...
			if (!(*Warnings & TEMP_HIGH)){				//if not logged
				LogEvent(EVENT_TEMP_HI_WARN, side);	//log it
				*Warnings |= TEMP_HIGH;					//flag as logged
			}
		}
		else if((int_16)tmpalarm < (int_16)((A2Data->TempLowAlarm[0] << 8) | A2Data->TempLowAlarm[1])){					//if a low alarm...
			if (!(*Alarms & TEMP_LOW)){					//if not logged...
				LogEvent(EVENT_TEMP_LO_ALARM, side);	//log it
				*Alarms |= TEMP_LOW;					//flag as logged
			}
		}
		else if((int_16)tmpalarm < (int_16)((A2Data->TempLowWarning[0] << 8) | A2Data->TempLowWarning[1])){					//if low warn...
			if (!(*Warnings & TEMP_LOW)){				//if not logged
				LogEvent(EVENT_TEMP_LO_WARN, side);	//log it
				*Warnings |= TEMP_LOW;					//flag as logged
			}
		}
		else{											//if no alarms or warns for temp
														//if at least 1 was logged
			if (((*Alarms & (TEMP_HIGH | TEMP_LOW)) | (*Warnings & (TEMP_HIGH | TEMP_LOW))) != 0){
				LogEvent(EVENT_TEMP_OK, side);		//log it
				*Alarms &= ~(TEMP_HIGH | TEMP_LOW);		//clear logged flags
				*Warnings &= ~(TEMP_HIGH | TEMP_LOW);
			}
		}
		tmpalarm = (A2Data->IntSupplyV[0] << 8) | A2Data->IntSupplyV[1];
		if(tmpalarm > ((A2Data->VoltageHighAlarm[0] << 8) | A2Data->VoltageHighAlarm[1])){//if a high alarm...
			if (!(*Alarms & VCC_HIGH)){				//if not logged...
				LogEvent(EVENT_VOLT_HI_ALARM, side);	//log it
				*Alarms |= VCC_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm > ((A2Data->VoltageHighWarning[0] << 8) | A2Data->VoltageHighWarning[1])){//if high warn...
			if (!(*Warnings & VCC_HIGH)){				//if not logged
				LogEvent(EVENT_VOLT_HI_WARN, side);	//log it
				*Warnings |= VCC_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->VoltageLowAlarm[0] << 8) | A2Data->VoltageLowAlarm[1])){					//if a low alarm...
			if (!(*Alarms & VCC_LOW)){					//if not logged...
				LogEvent(EVENT_VOLT_LO_ALARM, side);	//log it
				*Alarms |= VCC_LOW;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->VoltageLowWarning[0] << 8) | A2Data->VoltageLowWarning[1])){					//if low warn...
			if (!(*Warnings & VCC_LOW)){				//if not logged
				LogEvent(EVENT_VOLT_LO_WARN, side);	//log it
				*Warnings |= VCC_LOW;					//flag as logged
			}
		}
		else{											//if no alarms or warns for temp
														//if at least 1 was logged
			if (((*Alarms & (VCC_HIGH | VCC_LOW)) | (*Warnings & (VCC_HIGH | VCC_LOW))) != 0){
				LogEvent(EVENT_VOLT_OK, side);		//log it
				*Alarms &= ~(VCC_HIGH | VCC_LOW);		//clear logged flags
				*Warnings &= ~(VCC_HIGH | VCC_LOW);
			}
		}
		tmpalarm = (A2Data->TxBiasCurrent[0] << 8) | A2Data->TxBiasCurrent[1];
		if(tmpalarm > ((A2Data->BiasHighAlarm[0] << 8) | A2Data->BiasHighAlarm[1])){//if a high alarm...
			if (!(*Alarms & TX_BIAS_HIGH)){				//if not logged...
				LogEvent(EVENT_BIAS_HI_ALARM, side);	//log it
				*Alarms |= TX_BIAS_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm > ((A2Data->BiasHighWarning[0] << 8) | A2Data->BiasHighWarning[1])){//if high warn...
			if (!(*Warnings & TX_BIAS_HIGH)){				//if not logged
				LogEvent(EVENT_BIAS_HI_WARN, side);	//log it
				*Warnings |= TX_BIAS_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->BiasLowAlarm[0] << 8) | A2Data->BiasLowAlarm[1])){					//if a low alarm...
			if (!(*Alarms & TX_BIAS_LOW)){					//if not logged...
				LogEvent(EVENT_BIAS_LO_ALARM, side);	//log it
				*Alarms |= TX_BIAS_LOW;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->BiasLowWarning[0] << 8) | A2Data->BiasLowWarning[1])){					//if low warn...
			if (!(*Warnings & TX_BIAS_LOW)){				//if not logged
				LogEvent(EVENT_BIAS_LO_WARN, side);	//log it
				*Warnings |= TX_BIAS_LOW;					//flag as logged
			}
		}
		else{											//if no alarms or warns for temp
														//if at least 1 was logged
			if (((*Alarms & (TX_BIAS_HIGH | TX_BIAS_LOW)) | (*Warnings & (TX_BIAS_HIGH | TX_BIAS_LOW))) != 0){
				LogEvent(EVENT_BIAS_OK, side);		//log it
				*Alarms &= ~(TX_BIAS_HIGH | TX_BIAS_LOW);		//clear logged flags
				*Warnings &= ~(TX_BIAS_HIGH | TX_BIAS_LOW);
			}
		}
		tmpalarm = (A2Data->TxOutputPower[0] << 8) | A2Data->TxOutputPower[1];
		if(tmpalarm > ((A2Data->TXPowerHighAlarm[0] << 8) | A2Data->TXPowerHighAlarm[1])){//if a high alarm...
			if (!(*Alarms & TX_POWER_HIGH)){				//if not logged...
				LogEvent(EVENT_TX_PWR_HI_ALARM, side);	//log it
				*Alarms |= TX_POWER_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm > ((A2Data->TXPowerHighWarning[0] << 8) | A2Data->TXPowerHighWarning[1])){//if high warn...
			if (!(*Warnings & TX_POWER_HIGH)){				//if not logged
				LogEvent(EVENT_TX_PWR_HI_WARN, side);	//log it
				*Warnings |= TX_POWER_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->TXPowerLowAlarm[0] << 8) | A2Data->TXPowerLowAlarm[1])){					//if a low alarm...
			if (!(*Alarms & TX_POWER_LOW)){					//if not logged...
				LogEvent(EVENT_TX_PWR_LO_ALARM, side);	//log it
				*Alarms |= TX_POWER_LOW;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->TXPowerLowWarning[0] << 8) | A2Data->TXPowerLowWarning[1])){					//if low warn...
			if (!(*Warnings & TX_POWER_LOW)){				//if not logged
				LogEvent(EVENT_TX_PWR_LO_WARN, side);	//log it
				*Warnings |= TX_POWER_LOW;					//flag as logged
			}
		}
		else{											//if no alarms or warns for temp
														//if at least 1 was logged
			if (((*Alarms & (TX_POWER_HIGH | TX_POWER_LOW)) | (*Warnings & (TX_POWER_HIGH | TX_POWER_LOW))) != 0){
				LogEvent(EVENT_TX_PWR_OK, side);		//log it
				*Alarms &= ~(TX_POWER_HIGH | TX_POWER_LOW);		//clear logged flags
				*Warnings &= ~(TX_POWER_HIGH | TX_POWER_LOW);
			}
		}
		tmpalarm = (A2Data->RxInputPower[0] << 8) | A2Data->RxInputPower[1];
		if(tmpalarm > ((A2Data->RXPowerHighAlarm[0] << 8) | A2Data->RXPowerHighAlarm[1])){//if a high alarm...
			if (!(*Alarms & RX_POWER_HIGH)){				//if not logged...
				LogEvent(EVENT_RX_PWR_HI_ALARM, side);	//log it
				*Alarms |= RX_POWER_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm > ((A2Data->RXPowerHighWarning[0] << 8) | A2Data->RXPowerHighWarning[1])){//if high warn...
			if (!(*Warnings & RX_POWER_HIGH)){				//if not logged
				LogEvent(EVENT_RX_PWR_HI_WARN, side);	//log it
				*Warnings |= RX_POWER_HIGH;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->RXPowerLowAlarm[0] << 8) | A2Data->RXPowerLowAlarm[1])){					//if a low alarm...
			if (!(*Alarms & RX_POWER_LOW)){					//if not logged...
				LogEvent(EVENT_RX_PWR_LO_ALARM, side);	//log it
				*Alarms |= RX_POWER_LOW;					//flag as logged
			}
		}
		else if(tmpalarm < ((A2Data->RXPowerLowWarning[0] << 8) | A2Data->RXPowerLowWarning[1])){					//if low warn...
			if (!(*Warnings & RX_POWER_LOW)){				//if not logged
				LogEvent(EVENT_RX_PWR_LO_WARN, side);	//log it
				*Warnings |= RX_POWER_LOW;					//flag as logged
			}
		}
		else{											//if no alarms or warns for temp
														//if at least 1 was logged
			if (((*Alarms & (RX_POWER_HIGH | RX_POWER_LOW)) | (*Warnings & (RX_POWER_HIGH | RX_POWER_LOW))) != 0){
				LogEvent(EVENT_RX_PWR_OK, side);		//log it
				*Alarms &= ~(RX_POWER_HIGH | RX_POWER_LOW);		//clear logged flags
				*Warnings &= ~(RX_POWER_HIGH | RX_POWER_LOW);
			}
		}
	}
}

static float GetFloat(uint_8 *byte){
	float tmp;
	uint_8 *ptr;

	ptr = (uint_8*)&tmp;
	*ptr++ = *(byte + 3);
	*ptr++ = *(byte + 2);
	*ptr++ = *(byte + 1);
	*ptr = *byte;
	return tmp;
}

void DefaultSFPData(uint_8 side){
	struct SFPDataA0* A0data;
	struct sfpdata *data;

	if (side < SOURCE_NET_P_SFP || side > SOURCE_CUST_S_SFP)
		return;
	A0data = &SFPDataA0[side];
	data = &SFPStatus[side];
	if (side == SOURCE_NET_P_SFP)
		LEDState[NET_P_SPEED] = LED_OFF;
	else if (side == SOURCE_NET_S_SFP)
		LEDState[NET_S_SPEED] = LED_OFF;
	else if (side == SOURCE_CUST_P_SFP)
		LEDState[CUST_P_SPEED] = LED_OFF;
	else if (side == SOURCE_CUST_S_SFP)
		LEDState[CUST_S_SPEED] = LED_OFF;
	data->EventAlarmFlags = 0;
	data->EventWarnFlags = 0;
	data->Options = 0;
	data->SFPFlags &= ~(DDM_IMPLEMENTED | ADDR_CHANGE | CLEAR_FAULT);

	strncpy(A0data->VendorName, "                ", 16);
	A0data->Tranceiver[0] = 0;
	A0data->Tranceiver[1] = 0;
	A0data->Tranceiver[2] = 0;
	A0data->Tranceiver[3] = 0;
	A0data->Tranceiver[4] = 0;
	A0data->Tranceiver[5] = 0;
	A0data->Tranceiver[6] = 0;
	A0data->Tranceiver[7] = 0;
	A0data->RateIdentifier = 0;
	strncpy(A0data->VendorPN, "                ", 16);
	strncpy(A0data->VendorRev, "    ", 4);
	A0data->Connector = 0x7f;		//reserved/ unallocated
	A0data->Encoding = 7;			//reserved
	A0data->Length50um = 0;
	A0data->Length62_5um = 0;
	A0data->LengthCable = 0;
	A0data->LengthOM3 = 0;
	A0data->LengthSMF = 0;
	A0data->LengthSMFkm = 0;
	A0data->BRNominal = 0;
	A0data->DiagMonType = 0;
	A0data->EnhancedOptions = 0;
	A0data->SFF_8472Compilance = 0;
}

void DefaultSFPDataA2(uint_8 side){
	struct actualvalues *A2data;
	struct SFPDataA2 *raw;
	struct sfpdata *data;

	if (side < SOURCE_NET_P_SFP || side > SOURCE_CUST_S_SFP)
		return;
	A2data = &ActualValues[side];
	raw = &SFPDataA2[side];
	data = &SFPStatus[side];

	data->SFPFlags &= ~(FIRST_READ_A2 | CHANGE_DISABLE | CHANGE_RX_RATE | CHANGE_TX_RATE | CHANGE_POWER);
	data->FaultCount = 0;

	A2data->Bias = 0;
	A2data->IntTemp = 0;
	A2data->RXPower = 0;
	A2data->TXPower = 0;
	A2data->Voltage = 0;
	A2data->LaserTemp = 0;
	A2data->TECCurrent = 0;
	A2data->AlarmFlags = 0;
	A2data->WarnFlags = 0;
	memset(raw, 0, sizeof(struct SFPDataA2));
}

