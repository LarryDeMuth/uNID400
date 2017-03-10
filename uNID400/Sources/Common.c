/*
 * Common.c
 *
 *  Created on: Mar 19, 2016
 *      Author: Larry
 */

#include "config.h"
#include <mqx.h>
#include "extern.h"
#include "common.h"
#include <string.h>

void GetCompliance(char* str, uint_8 type);
void GetTranceiver(char* strptr, uint_8* type);
void GetConnector(char *str, uint_8 type);
void GetEncoding(char *str, uint_8 type);
void GetBitRate(char *str, uint_8 rate);
void FormatDate(char *str, LDD_RTC_TTime date);
void FormatTime(char *str, LDD_RTC_TTime time);
void inttoascii(char* str, uint32_t num);
void GetString(uint_8 *addr, char* str);
uint_8 GetIP(uint_16* num, char* dat);
uint_8 itoa(char* ch, uint_32 num, bool signd);

void GetCompliance(char* str, uint_8 type){
	if (type > COMPLIANCE_UNDEFINED && type < COMPLIANCE_INVALID){
		strcat(str, "SFF-8472 Rev ");
		strcat(str, Compliance[type]);
	}
	else
		strcat(str, "                  ");
}

void GetTranceiver(char *strptr, uint_8* type){
	uint8 count = 0, temp, added = 0, str[33], sonet = 0;

	str[0] = 0;
	if (*type != 0){
		temp = *type;
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		strcat(str, TransceiverType0[count]);
		added = strlen(str);
	}
	type++;
	if (*type != 0){
		temp = *type;
		sonet = (temp >> 3) & 0x03;			//get SONET reach specifier bits
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		if (count != 3 && count != 4){		//these further describe SONET. not enough room so dont include
			if (added)
				strcat(str, ", ");
			strcat(str, TransceiverType1[count]);
			if (count == 0 || count == 5)		//get sonet short reach
				strcat(str, SONETShortReach[sonet]);
			else if (count == 1)				//get sonet inter reach
				strcat(str, SONETInterReach[sonet]);
			else if (count == 2)				//get sonet long reach
				strcat(str, SONETLongReach[sonet]);
			added = strlen(str);
		}
	}
	type++;
	if (*type != 0 && added  < 15){
		temp = *type;
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		if (count != 3 && count != 7){		//these are not used
			if (added)
				strcat(str, ", ");
			strcat(str, TransceiverType2[count]);
			if (count == 0 || count == 4)
				strcat(str, SONETShortReach[sonet]);
			else if (count == 1 || count == 5)				//get sonet inter reach
				strcat(str, SONETInterReach[sonet]);
			else if (count == 2 || count == 6)				//get sonet long reach
				strcat(str, SONETLongReach[sonet]);
			added = strlen(str);
		}
	}
	type++;
	if (*type != 0 && added  < 15){
		temp = *type;
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		if (added)
			strcat(str, ", ");
		strcat(str, TransceiverType3[count]);
		added = strlen(str);
	}
/*			//dont see an reason to add these
	type++;
	if (*type != 0 && added  < 28){
		temp = *type;
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		if (added)
			strcat(str, ", ");
		strcat(str, TransceiverType4[count]);
		added = strlen(str);
	}
	type++;
	if (*type != 0 && added  < 23){
		temp = *type;
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		if (count > 1){		//these are not used
			if (added)
				strcat(str, ", ");
			strcat(str, TransceiverType5[count]);
			added++;
		}
	}
	type++;
	if (*type != 0 && added  < 28){
		temp = *type;
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		if (count != 1){		//these are not used
			if (added)
				strcat(str, ", ");
			strcat(str, TransceiverType6[count]);
			added = strlen(str);
		}
	}
	type++;
	if (*type != 0 && added  < 21){
		temp = *type;
		for (count = 0; count < 8; count++){
			if (temp & 0x01)
				break;
			temp >>= 1;
		}
		if (count != 1){		//these are not used
			if (added)
				strcat(str, ", ");
			strcat(str, TransceiverType7[count]);
			added = strlen(str);
		}
	}
	type++;
*/
	for (;added < 32; added++)		//fill in the remaining with spaces to overwrite any old data
		str[added] = ' ';
	str[32] = 0;
	strcat(strptr, str);
}

void GetConnector(char* str, uint_8 type){
	if (type > 0x7f){
		strcat(str, ConnectorType[11]);
	}
	else{
		switch(type){
			case 0:
				strcat(str, ConnectorType[0]);
			break;
			case 0x6:
				strcat(str, ConnectorType[1]);
			break;
			case 0x7:
				strcat(str, ConnectorType[2]);
			break;
			case 0x8:
				strcat(str, ConnectorType[3]);
			break;
			case 0x9:
				strcat(str, ConnectorType[4]);
			break;
			case 0xa:
				strcat(str, ConnectorType[5]);
			break;
			case 0xb:
				strcat(str, ConnectorType[6]);
			break;
			case 0xc:
				strcat(str, ConnectorType[7]);
			break;
			case 0x20:
				strcat(str, ConnectorType[8]);
			break;
			case 0x21:
				strcat(str, ConnectorType[9]);
			break;
			case 0x22:
				strcat(str, ConnectorType[10]);
			break;
			default:
				strcat(str, ConnectorType[12]);
			break;
		}
	}
}

void GetEncoding(char *str, uint_8 type){
	if (type < 7)
		strcat(str, EncodingType[type]);
	else
		strcat(str, EncodingType[7]);
}

void GetBitRate(char *str, uint_8 rate){
	if (rate == 0)
		strcat(str, "            ");
	else
		sprintf(str + strlen(str), "%d00 MB/s  ", rate);
}



void FormatDate(char *str, LDD_RTC_TTime date){
	inttoascii(str, date.Month);
	strcat(str, "/");
	inttoascii(str, date.Day);
	strcat(str, "/");
	inttoascii(str, date.Year);
}

void FormatTime(char *str, LDD_RTC_TTime time){
	inttoascii(str, time.Hour);
	strcat(str, ":");
	inttoascii(str, time.Minute);
	strcat(str, ":");
	inttoascii(str, time.Second);
}

void inttoascii(char* str, uint32_t num){
	int tmp, len, startlen;

	startlen = len = strlen(str);
	if (num > 999){
		tmp = num / 1000;
		num -= (tmp * 1000);
		*(str + len++) = tmp + '0';
	}
	if (num > 99){
		tmp = num / 100;
		num -= (tmp * 100);
		*(str + len++) = tmp + '0';
	}
	if (startlen != len)
		*(str + len++) = '0';
	if (num > 9){
		tmp = num / 10;
		num -= (tmp * 10);
	}
	else
		tmp = 0;
	*(str + len++) = tmp + '0';
	*(str + len++) = num + '0';
	*(str + len) = 0;
}

uint_32 asciitoint(char* num, uint_8 len){
	uint_32 tmp = 0;

	while (len){
		tmp *= 10;
		tmp += *(num) & 0x0f;
		len--;
		num++;
	}
	return tmp;
}

uint_8 itoa(char* ch, uint_32 num, bool signd)
{
	char tmp;
	uint_8 count = 0;
	uint_32 div = 1000000000;
	bool addzero = 0;

	if (signd){
		if (num & 0x80000000){
			*ch = '-';
			ch++;
			count++;
			num ^= 0xffffffff;
			num++;
		}
	}
	while (div > 1){
		tmp = num / div;
		num -= tmp * div;
		*ch = tmp + '0';
		if (tmp != 0){
			ch++;
			addzero = 1;
			count++;
		}
		else if (tmp == 0 && addzero){
			ch++;
			count++;
		}
		div /= 10;
	}
	*ch = num + '0';
	ch++;
	count++;
	*ch = 0;
	return count;
}

