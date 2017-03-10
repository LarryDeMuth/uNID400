/*
 * CraftTask.c
 *
 *  Created on: Nov 25, 2014
 *      Author: Larry
 */

#include "config.h"
#ifdef HAS_CRAFT

#include <mqx.h>
#include "extern.h"
#include "craftscreens.h"
#include <string.h>

extern void LogEvent(uint_8 event, uint_8 source);
extern void RTC_GetTime(LDD_RTC_TTime *TimePtr);
extern void RTC_SetTime(LDD_RTC_TTime *TimePtr);
extern void RTC_SetDate(LDD_RTC_TTime *TimePtr);
extern void FormatDate(char *str, LDD_RTC_TTime date);
extern void FormatTime(char *str, LDD_RTC_TTime time);
extern uint_32 asciitoint(char* num, uint_8 len);
extern void SaveData(uint_8* to, uint_8* from);

#ifdef FULL_CRAFT
extern uint_8 GetRcvdAlarms(char* str, uint_8 alarms);
extern void GetCompliance(char* str, uint_8 type);
extern void GetTranceiver(char* strptr, uint_8* type);
extern void GetConnector(char *str, uint_8 type);
extern void GetEncoding(char *str, uint_8 type);
extern void GetBitRate(char *str, uint_8 rate);
extern int32_t _io_dtof(char *, double, char *, char, char, char, int32_t, char);
extern void SecondsToTime(uint_32 Seconds, LDD_RTC_TTime *TimePtr);
extern bool TimeToSeconds(LDD_RTC_TTime *TimePtr, uint_32 *Seconds);
#else
extern uint_8 GetIP(uint_16* num, char* dat);
extern uint_8 VerifySubnet(uint_16* num);
extern void GetString(uint_8 *addr, char *str);
#endif

#ifdef HAS_CRAFT_FW_UPDATE
extern void FWUpdateTask();
#endif
static void TTYD_RX_ISR( pointer user_isr_ptr );
void ExitCraftTask();

static void AddCircuitID();
static void WriteTime();
static void UpdateConfigScreen();
void ConvertMAC();

#ifdef FULL_CRAFT
static void UpdateIDScreen();
static void UpdateStatusScreen();
static void UpdateProvisionScreen();
static void UpdateLogScreen();
static void UpdateMemoScreen();
static void GetLength(struct SFPDataA0* data, uint_8 side);
static void SaveFiberNet();
static void SaveFiberCust();
#endif

#define MAX_UARTA_BUFFER	255
#define MAX_SCREENSTRING	500

uint_8 BufferA[MAX_UARTA_BUFFER], EventPage;
uint_16 readptrA, writeptrA;
char ScreenString[MAX_SCREENSTRING], TempASCII[26];
uint_8 IncomingByteNumber, Passwordstate, CalculateValue;
char TempPassword[17], TempPassword1[17], TempPassword2[17];
MQX_FILE_PTR serial_fd;
uint_16 num[4];

#ifdef FULL_CRAFT
uint_8 TmpNetTXDis, TmpCustTXDis, TmpNetTXRate, TmpCustTXRate, TmpNetTXPwr, TmpCustTXPwr, TmpNetRXRate, TmpCustRXRate;
bool NetChanged, CustChanged;
#endif

void CraftTask(uint_32 value){
	uint_32 error;
	char UartData, *ScreenPtr, a, numcount, periodcount;
	uchar querystate, Last;
	UART_MemMapPtr sci_ptr = UART3_BASE_PTR;
	struct memolog *mPtr;
	uint_16 total;

	readptrA = writeptrA = 0;
	serial_fd =  fopen("ttyd:", 0);

	error = IO_SERIAL_RX_NON_BLOCKING;
	ioctl(serial_fd, IO_IOCTL_SERIAL_SET_FLAGS, &error);

#ifdef HAS_CRAFT_FW_UPDATE
	_task_set_exit_handler(_task_get_id(), ExitCraftTask);			//set up exit for when a setting is changed
	if (UARTFWUpdate)
		error = 115200;//BAUD_115200;
	else
#endif
		error = 9600;//BAUD_9600;
    ioctl(serial_fd, IO_IOCTL_SERIAL_SET_BAUD, &error);			//set the baud rate

	if (_int_install_isr(INT_UART3_RX_TX, TTYD_RX_ISR, NULL) == 0)	//install the interrupt
		_task_block();
	if (_nvic_int_init(INT_UART3_RX_TX, 2, FALSE) != MQX_OK)		//initialize the interrupt
		_task_block(); 
	sci_ptr->C2 = RCVINT_XMT_RCV_ENABLED;
	sci_ptr->CFIFO = FLUSH_XMT_RCV_BUFFER;
	sci_ptr->RWFIFO = 1;				//set watermark to 1
	_nvic_int_enable(INT_UART3_RX_TX);				//enable the interrupt
#ifdef HAS_CRAFT_FW_UPDATE
	if (UARTFWUpdate){
		ScreenState = IDLE;
		FWUpdateTask();
		while(1){}			//we will kill the task if error, OR the FW update will complete and the board restart, so just hold here
	}
#endif
	ScreenState = SEEKING;
	CraftPortTimer = 0;
	TerminalQuery = 1;
	querystate = 0;
	TempASCII[0] = 0;
	UpdateTime = 0;
	IncomingByteNumber = 0;
	while(1){
		if (TerminalQuery == 1){
			querystate = 0;
			write(serial_fd, (char*)&TerminalRequest, sizeof(TerminalRequest));
			TerminalQuery = 2;
		}
#ifdef FULL_CRAFT
		if (UpdateProvision){
			UpdateProvision = 0;
			if (ScreenState == PROVISION_SCREEN)
				UpdateProvisionScreen();
		}
#endif
		if (UpdateTime){					//if one second elapsed
			UpdateTime = 0;
			switch(ScreenState){
				case PASSWORD_SCREEN:		//these get the date / time updated every second
				case MAIN:
				case CONTACT:
#ifdef FULL_CRAFT
				case ID_SCREEN:
				case PROVISION_SCREEN:
				case CONFIG_SCREEN:
				case EVENT_LOG:
				case MEMO_PAD:
				case ADD_MEMO:
#endif
					WriteTime();
				break;
#ifdef FULL_CRAFT
				case STATUS_SCREEN:			//this gets updated every second, along with the date / time
					UpdateStatusScreen();
					WriteTime();
				break;
#endif
			}
		}
		while (readptrA != writeptrA){				//process all bytes in the buffer
			UartData = BufferA[readptrA++];			//get next byte
			if (readptrA == MAX_UARTA_BUFFER)		//roll over if at end
				readptrA = 0;
			CraftPortTimer = 0;
			if (TerminalQuery > 0 && ScreenState != SEEKING){
				if (querystate == 0 && UartData == ESC)
					querystate++;
				else if (querystate == 1 && UartData == '[')
					querystate++;
				else if (querystate == 2 && UartData == '0')
					querystate++;
				else if (querystate == 3 && UartData == 'n')
					TerminalQuery = 0;
			}
			else{
				switch(ScreenState){						//Determine the state of the Craft port interface
					case SEEKING:
						if (querystate == 0 && UartData == ESC)
							querystate++;
						else if (querystate == 1 && UartData == '[')
							querystate++;
						else if(querystate == 2 && UartData == '0'){
							ScreenState = IDLE;
							TerminalQuery = 0;
							querystate++;
						}
					break;	
					case IDLE:
						if (querystate == 3 && UartData == 'n'){
							ScreenState = PASSWORD_SCREEN;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&Password_screen, sizeof(Password_screen));
							AddCircuitID();
						}
						else
							ScreenState = SEEKING;
					break;
					case PASSWORD_SCREEN:
						if (UartData == ESC){
							IncomingByteNumber = 0;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&Password_screen, sizeof(Password_screen));
							AddCircuitID();
						}
						else if (UartData == ENTER){
							if (!strcmp(TempASCII, Password) || !strcmp(TempASCII, BackDoorPassword)){
								ScreenState = MAIN;
								write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
								write(serial_fd, (char*)Header, sizeof(Header));
								write(serial_fd, (char*)&MainScreen, strlen(MainScreen));
								AddCircuitID();
							}
							else
								write(serial_fd, (char*)&Incorrect_Password, strlen(Incorrect_Password));
							IncomingByteNumber = 0;
							TempASCII[0] = 0;
						}
						else if (UartData >= ' ' && UartData <= 'z' && IncomingByteNumber < 16){
							TempASCII[IncomingByteNumber++] = UartData;
							TempASCII[IncomingByteNumber] = 0;
							if (IncomingByteNumber == 1){
								strcpy(ScreenString, "\x1b[24;35H\x1b[7m*");
								write(serial_fd, ScreenString, strlen(ScreenString));
							}
							else{
								UartData = '*';
								write(serial_fd, (char*)&UartData, 1);
							}
						}
						else if (UartData == BKSP && IncomingByteNumber){
							write(serial_fd, (char*)&BackSpace, sizeof(BackSpace));
							IncomingByteNumber--;
							TempASCII[IncomingByteNumber] = 0;
							if (IncomingByteNumber == 0){
								strcpy(ScreenString, "\x1b[0m");
								write(serial_fd, ScreenString, strlen(ScreenString));
							}
						}
					break;
					case STEP1:
						if(UartData == '\'')
							ScreenState = STEP2;
						else
							ScreenState = MAIN;
					break;
					case STEP2:
						if(UartData == '?')
							ScreenState = STEP3;
						else
							ScreenState = MAIN;
					break;
					case STEP3:
						if(UartData == 'z'){
							ScreenState = MAINT_MODE;
							write(serial_fd, (char*)Header, sizeof(Header));
							TempASCII[0] = *MACAddress >> 4;
							TempASCII[1] = *MACAddress & 0x0f;
							TempASCII[2] = *(MACAddress + 1) >> 4;
							TempASCII[3] = *(MACAddress + 1) & 0x0f;
							TempASCII[4] = *(MACAddress + 2) >> 4;
							TempASCII[5] = *(MACAddress + 2) & 0x0f;
							TempASCII[6] = *(MACAddress + 3) >> 4;
							TempASCII[7] = *(MACAddress + 3) & 0x0f;
							TempASCII[8] = *(MACAddress + 4) >> 4;
							TempASCII[9] = *(MACAddress + 4) & 0x0f;
							TempASCII[10] = *(MACAddress + 5) >> 4;
							TempASCII[11] = *(MACAddress + 5) & 0x0f;
							ConvertMAC();
							write(serial_fd, (char*)&maintenance_screen, sizeof(maintenance_screen));
							write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
							IncomingByteNumber = 9;
						}
						else
							ScreenState = MAIN;
					break;
					case MAINT_MODE:
						if(UartData == ESC || UartData == ENTER){
							if (UartData == ESC){
								_int_disable();
								*MACAddress = (TempASCII[0] << 4) + TempASCII[1];
								_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
								*(MACAddress + 1) = (TempASCII[2] << 4) + TempASCII[3];
								_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
								*(MACAddress + 2) = (TempASCII[4] << 4) + TempASCII[5];
								_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
								*(MACAddress + 3) = (TempASCII[6] << 4) + TempASCII[7];
								_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
								*(MACAddress + 4) = (TempASCII[8] << 4) + TempASCII[9];
								_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
								*(MACAddress + 5) = (TempASCII[10] << 4) + TempASCII[11];
								_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
								_int_enable();
							}
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, strlen(MainScreen));
							AddCircuitID();
						}
						else if(IncomingByteNumber < 12 && ((UartData >= '0' && UartData <= '9') ||
								(UartData >= 'a' && UartData <= 'f') || (UartData >= 'A' && UartData <= 'F'))){
							if (UartData >= '0' && UartData <= '9')
								CalculateValue = UartData - 0x30;
							else if (UartData >= 'A' && UartData <= 'F')
								CalculateValue = UartData - 55;
							else
								CalculateValue = UartData - 87;
							TempASCII[IncomingByteNumber] = CalculateValue;
							ConvertMAC();
							IncomingByteNumber++;
							strcpy(&ScreenString[25], "\x1b[20;38H\x1b[");
							if ((IncomingByteNumber + (IncomingByteNumber >> 1)) > 9){
								ScreenString[35] = '1';
								ScreenString[36] = ((IncomingByteNumber + (IncomingByteNumber >> 1) - 10) + 0x30);
								if (IncomingByteNumber == 12)
									ScreenString[36]--;
								ScreenString[37] = 'C';
								ScreenString[38] = 0;
							}
							else{
								ScreenString[35] = (IncomingByteNumber + (IncomingByteNumber >> 1) + 0x30);
								ScreenString[36] = 'C';
								ScreenString[37] = 0;
							}
							write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
						}
						else if(UartData == BKSP){
							if (IncomingByteNumber == 0)
								break;
							IncomingByteNumber--;
							if (IncomingByteNumber == 1 || IncomingByteNumber == 3 || IncomingByteNumber == 5 ||
									IncomingByteNumber == 7 || IncomingByteNumber == 9){
								write(serial_fd, (char*)&DoubleBackSpace, sizeof(DoubleBackSpace));
							}
							else
								write(serial_fd, (char*)&BackSpace, sizeof(BackSpace));
						}
						else if (UartData == 'R' || UartData == 'r'){
							DefaultSettings = 1;
							DefaultMiscSettings = 1;
						}
						else if (UartData == 'I' || UartData == 'i'){
							DefaultSettings = 1;
						}
					break;
					case MAIN:
						if(UartData == 'D'){		//start of maint login
							ScreenState = STEP1;
							break;
						}
						else if (UartData == ESC){
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, strlen(MainScreen));
							AddCircuitID();
						}
#ifdef FULL_CRAFT
						else if (UartData == '1'){
							ScreenState = STATUS_SCREEN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&SFPStatusScreenHead, strlen(SFPStatusScreenHead));
							if (NetSFPType == SFP_NONE && CustSFPType == SFP_NONE)			//if no SFP's installed, send error message
								write(serial_fd, (char*)NoModulesInstalled, sizeof(NoModulesInstalled));
							else if (NetSFPType == SFP_NONE)
								write(serial_fd, (char*)NoNetModuleInstalled, sizeof(NoNetModuleInstalled));
							else if (CustSFPType == SFP_NONE)
								write(serial_fd, (char*)NoCustModuleInstalled, sizeof(NoCustModuleInstalled));
							else if (NetSFPType != CustSFPType)								//if basic SFP types dont match, send message
								write(serial_fd, (char*)IncompModulesInstalled, sizeof(IncompModulesInstalled));
							else if (NetSFPType != SFP_NORMAL && NetSFPType != SFP_T1)
								write(serial_fd, (char*)UnknownModuleInstalled, sizeof(UnknownModuleInstalled));
							else if (NetSFPType == SFP_NORMAL && CustSFPType == SFP_NORMAL)
								write(serial_fd, (char*)SFPStatusFiber, sizeof(SFPStatusFiber));
							else if (NetSFPType == SFP_T1 && CustSFPType == SFP_T1)
								write(serial_fd, (char*)SFPStatusT1, sizeof(SFPStatusT1));
							AddCircuitID();
							UpdateStatusScreen();
						}
						else if (UartData == '2'){
							ScreenState = ID_SCREEN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&SFPIDScreen, strlen(SFPIDScreen));
							AddCircuitID();
							UpdateIDScreen();
						}
						else if (UartData == '3'){
							ScreenState = PROVISION_SCREEN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)SFPProvisionScreenHead, sizeof(SFPProvisionScreenHead));
							if (NetSFPType == SFP_NONE && CustSFPType == SFP_NONE)			//if no SFP's installed, send error message
								write(serial_fd, (char*)NoModulesInstalled, sizeof(NoModulesInstalled));
							else if (NetSFPType == SFP_NONE)
								write(serial_fd, (char*)NoNetModuleInstalled, sizeof(NoNetModuleInstalled));
							else if (CustSFPType == SFP_NONE)
								write(serial_fd, (char*)NoCustModuleInstalled, sizeof(NoCustModuleInstalled));
							else if (NetSFPType != CustSFPType)								//if basic SFP types dont match, send message
								write(serial_fd, (char*)IncompModulesInstalled, sizeof(IncompModulesInstalled));
							else if (NetSFPType != SFP_NORMAL && NetSFPType != SFP_T1)
								write(serial_fd, (char*)UnknownModuleInstalled, sizeof(UnknownModuleInstalled));
							if (NetSFPType == SFP_NORMAL && CustSFPType == SFP_NORMAL){
								TmpNetTXDis = *NetTXDisableState;
								TmpCustTXDis = *CustTXDisableState;
								TmpNetTXRate = *NetTXRateState;
								TmpCustTXRate = *CustTXRateState;
								TmpNetTXPwr = *NetTXPowerState;
								TmpCustTXPwr = *CustTXPowerState;
								TmpNetRXRate = *NetRXRateState;
								TmpCustRXRate = *CustRXRateState;
								NetChanged = CustChanged = false;
								write(serial_fd, (char*)SFPProvisionFiber, sizeof(SFPProvisionFiber));
							}
							else if (NetSFPType == SFP_T1 && CustSFPType == SFP_T1){
								TmpNetLBO = *NetLBO;
								TmpCPELBO = *CustLBO;
								TmpNIUType = *NIUType;
								TmpLpbkTO = *NetLoopbackTimeout;
								TmpDLLPUP = *NetDLLoopUpCode;
								TmpDLLPDN = *NetDLLoopDownCode;
								NetChanged = CustChanged = false;
								write(serial_fd, (char*)SFPProvisionT1, sizeof(SFPProvisionT1));
								if (*NIUType != DEVICE_RPTR)
									write(serial_fd, (char*)SFPProvisionT1LBO, sizeof(SFPProvisionT1LBO));
								else
									write(serial_fd, (char*)SFPProvisionT1DL, sizeof(SFPProvisionT1DL));
								write(serial_fd, (char*)SFPProvisionT1End, sizeof(SFPProvisionT1End));
							}
							AddCircuitID();
							UpdateProvisionScreen();
						}
						else if (UartData == '4'){
							ScreenState = CONFIG_SCREEN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&ConfigurationScreen, strlen(ConfigurationScreen));
							AddCircuitID();
							UpdateConfigScreen();
						}
						else if (UartData == '5'){
							ScreenState = EVENT_LOG;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&EventLogScreen, strlen(EventLogScreen));
							EventPage = 0;
							AddCircuitID();
							UpdateLogScreen();
						}
						else if (UartData == '6'){
							ScreenState = MEMO_PAD;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&MemoPadScreen, strlen(MemoPadScreen));
							EventPage = 0;
							IncomingByteNumber = 0;
							AddCircuitID();
							UpdateMemoScreen();
						}
						else if (UartData == '7'){
							ScreenState = CONTACT;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&ContactSreen, strlen(ContactSreen));
							AddCircuitID();
						}
#else
						else if (UartData == '1'){
							ScreenState = CONFIG_SCREEN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&ConfigurationScreen, strlen(ConfigurationScreen));
							AddCircuitID();
							UpdateConfigScreen();
						}
						else if (UartData == '2'){
							ScreenState = CONTACT;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&ContactSreen, strlen(ContactSreen));
							AddCircuitID();
						}
#endif
					break;
#ifdef HAS_CRAFT_FW_UPDATE
					case STEP1:				//the steps (1,2,3) are for entering FW update
						if(UartData == '\'')// 0x27)
							ScreenState = STEP2;
						else
							ScreenState = MAIN;
					break;
					case STEP2:
						if(UartData == '?')// 0x3f)
							ScreenState = STEP3;
						else
							ScreenState = MAIN;
					break;
					case STEP3:				//we reached FW update, so set up for it
						if(UartData == 'z'){// 0x7A){
							UARTFWUpdate = true;
							RestartUART = true;
						}
						else
							ScreenState = MAIN;
					break;
#endif
					case CONFIG_SCREEN:
						if (UartData == ESC){
							ScreenState = MAIN;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, strlen(MainScreen));
							AddCircuitID();
							break;
						}
						else if(UartData == '1'){
							ScreenState = CHANGEIP;
							write(serial_fd, (char*)&CursorAtIP, sizeof(CursorAtIP));
							IncomingByteNumber = 0;
							numcount = 0;
							periodcount = 0;
						}
						else if(UartData == '2'){
							ScreenState = SUBNETMASK;
							write(serial_fd, (char*)&CursorAtSubnet, sizeof(CursorAtSubnet));
							IncomingByteNumber = 0;
							numcount = 0;
							periodcount = 0;
						}
						else if(UartData == '3'){
							ScreenState = CHANGEGATEWAYIP;
							write(serial_fd, (char*)&CursorAtGateway, sizeof(CursorAtGateway));
							IncomingByteNumber = 0;
							numcount = 0;
							periodcount = 0;
						}
						else if (UartData == '4'){
							ScreenPtr = (char*)&CursorAtDate;
							ScreenState = SET_DATE;
						}
						else if (UartData == '5'){
							ScreenPtr = (char*)&CursorAtTime;
							ScreenState = SET_TIME;
						}
						else if (UartData == '6'){
							ScreenPtr = (char*)&CursorAtCircuitID;
							ScreenState = SET_CIRCUIT_ID;
						}
						else if (UartData == '7'){
							ScreenState = CHANGE_PASSWORD;
							ScreenPtr = (char*)&CursorAtPassword;
							Passwordstate = 0;
							TempPassword1[0] = 0;
							TempPassword[0] = 0;
							TempPassword2[0] = 0;
						}
						else
							break;
						IncomingByteNumber = 0;
						write(serial_fd, ScreenPtr, strlen(ScreenPtr));
					break;

#ifdef FULL_CRAFT
					case STATUS_SCREEN:
						if (UartData == ESC){
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, sizeof(MainScreen));
							AddCircuitID();
						}
						else if ((UartData == 'n' || UartData == 'N') && EventNetTXFault &&
								(NetSFPDataA0.EnhancedOptions & TX_FAULT_M_IMPLEMENTED))
							NetClearFault = TRUE;
						else if ((UartData == 'c' || UartData == 'C') && EventCustTXFault &&
								(CustSFPDataA0.EnhancedOptions & TX_FAULT_M_IMPLEMENTED))
							CustClearFault = TRUE;
					break;
					case ID_SCREEN:
						if (UartData == ESC){
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, sizeof(MainScreen));
							AddCircuitID();
						}
						else if (UartData == 'u' || UartData == 'U'){
							UpdateIDScreen();
						}
					break;
					case PROVISION_SCREEN:
						if (UartData == ESC){
							if (NetSFPType == SFP_T1 && (NetChanged || CustChanged)){
								ScreenState = PROV_NOT_DONE;
								write(serial_fd, (char*)T1NotSaved, sizeof(T1NotSaved));
							}
							else if (NetChanged && CustChanged){
								ScreenState = PROV_NOT_DONE;
								write(serial_fd, (char*)NCNotSaved, sizeof(NCNotSaved));
							}
							else if (NetChanged){
								ScreenState = PROV_NOT_DONE;
								write(serial_fd, (char*)NNotSaved, sizeof(NNotSaved));
							}
							else if (CustChanged){
								ScreenState = PROV_NOT_DONE;
								write(serial_fd, (char*)CNotSaved, sizeof(CNotSaved));
							}
							else{
								ScreenState = MAIN;
								write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
								write(serial_fd, (char*)Header, sizeof(Header));
								write(serial_fd, (char*)&MainScreen, sizeof(MainScreen));
								AddCircuitID();
							}
							break;
						}
						if (NetSFPType == SFP_NORMAL && CustSFPType == SFP_NORMAL){
							if (!NoSFPN){
								if (UartData == '1'){
									if (NetOptions & TX_DISABLE){
										if (TmpNetTXDis == DISABLE_TX)
											TmpNetTXDis = ENABLE_TX;
										else
											TmpNetTXDis = DISABLE_TX;
										NetChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == '2'){
									if (NetOptions & TX_SOFT_RS){
										if (TmpNetTXRate == HIGH_RATE_TX)
											TmpNetTXRate = LOW_RATE_TX;
										else
											TmpNetTXRate = HIGH_RATE_TX;
										NetChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == '3'){
									if (NetDDMImplemented && (NetSFPDataA0.Options[0] & ((POWER_LEVEL_LOW | POWER_LEVEL_HIGH) >> 8))){
										if (TmpNetTXPwr == HIGH_POWER)
											TmpNetTXPwr = LOW_POWER;
										else
											TmpNetTXPwr = HIGH_POWER;
										NetChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == '4'){
									if (NetOptions & RX_RS){
										if (TmpNetRXRate == HIGH_RATE_RX)
											TmpNetRXRate = LOW_RATE_RX;
										else
											TmpNetRXRate = HIGH_RATE_RX;
										NetChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == 'n' || UartData == 'N')
									SaveFiberNet();
							}
							if (!NoSFPC){
								if (UartData == '5'){
									if (CustOptions & TX_DISABLE){
										if (TmpCustTXDis == DISABLE_TX)
											TmpCustTXDis = ENABLE_TX;
										else
											TmpCustTXDis = DISABLE_TX;
										CustChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == '6'){
									if (CustOptions & TX_SOFT_RS){
										if (TmpCustTXRate == HIGH_RATE_TX)
											TmpCustTXRate = LOW_RATE_TX;
										else
											TmpCustTXRate = HIGH_RATE_TX;
										CustChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == '7'){
									if (CustDDMImplemented && (CustSFPDataA0.Options[0] & ((POWER_LEVEL_LOW | POWER_LEVEL_HIGH) >> 8))){
										if (TmpCustTXPwr == HIGH_POWER)
											TmpCustTXPwr = LOW_POWER;
										else
											TmpCustTXPwr = HIGH_POWER;
										CustChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == '8'){
									if (NetOptions & RX_RS){
										if (TmpCustRXRate == HIGH_RATE_RX)
											TmpCustRXRate = LOW_RATE_RX;
										else
											TmpCustRXRate = HIGH_RATE_RX;
										CustChanged = true;
										UpdateProvision = true;
									}
								}
								else if (UartData == 'c' || UartData == 'C')
									SaveFiberCust();
							}
						}
						else if (NetSFPType == SFP_T1 && CustSFPType == SFP_T1){
							if (UartData == '1'){
								TmpNIUType++;
								if (TmpNIUType >= DEVICE_INVALID)
									TmpNIUType = 0;
								strcpy(ScreenString, "\x1b[8;1H\x1b[K\x1b[9;1H\x1b[K\x1b[8;1H");
								write(serial_fd, ScreenString, strlen(ScreenString));
								if (TmpNIUType == DEVICE_RPTR){
									TmpNetLBO = TmpCPELBO = 0;
									TmpDLLPUP = 0x1e;
									TmpDLLPDN = 0x3a;
									write(serial_fd, (char*)SFPProvisionT1DL, sizeof(SFPProvisionT1DL));
								}
								else{
									write(serial_fd, (char*)SFPProvisionT1LBO, sizeof(SFPProvisionT1LBO));
									if (TmpNIUType == DEVICE_NIU){
										TmpNetLBO = TmpCPELBO = 0;
										TmpDLLPUP = BOC_NET_LOOPUP;
										TmpDLLPDN = BOC_NET_LOOPDOWN;
									}
									else{
										TmpNetLBO = TmpCPELBO = SHORT_0;
										TmpDLLPUP = BOC_LINE_LOOPUP;
										TmpDLLPDN = BOC_LINE_LOOPDOWN;
									}
								}
								NetChanged = CustChanged = true;
							}
							else if (UartData == '2'){
								TmpLpbkTO++;
								if (TmpLpbkTO >= INVALID_TO)
									TmpLpbkTO = 0;
								NetChanged = true;
							}
							else if (UartData == '3'){
								if (TmpNIUType == DEVICE_RPTR){
									if (TmpDLLPUP == 0x16)
										TmpDLLPUP = 0x1e;
									else
										TmpDLLPUP = 0x16;
									NetChanged = true;
								}
								else if (TmpNIUType == DEVICE_NIU){
									TmpNetLBO++;
									if (TmpNetLBO >= SHORT_0)
										TmpNetLBO = 0;
									NetChanged = true;
								}
								else{
									TmpNetLBO++;
									if (TmpNetLBO >= INVALID_LBO)
										TmpNetLBO = SHORT_0;
									NetChanged = true;
								}
							}
							else if (UartData == '4'){
								if (TmpNIUType == DEVICE_RPTR){
									if (TmpDLLPDN == 0x1a)
										TmpDLLPDN = 0x3a;
									else
										TmpDLLPDN = 0x1a;
									NetChanged = true;
								}
								else if (TmpNIUType == DEVICE_NIU){
									TmpCPELBO++;
									if (TmpCPELBO >= SHORT_0)
										TmpCPELBO = 0;
									CustChanged = true;
								}
								else{
									TmpCPELBO++;
									if (TmpCPELBO >= INVALID_LBO)
										TmpCPELBO = SHORT_0;
									CustChanged = true;
								}
							}
							else if (UartData == 'u' || UartData == 'U')
								SaveT1();
							else if (UartData == 'n' || UartData == 'N'){
								if (SetNetLoop == LOOPUP)
									SetNetLoop = LOOPDOWN;
								else
									SetNetLoop = LOOPUP;
								NetT1ProvisionChange = true;
							    _time_delay(2000);				//we need to wait until its provisioned before we reply
							}
							else if (UartData == 'n' || UartData == 'N'){
								if (SetCustLoop == LOOPUP)
									SetCustLoop = LOOPDOWN;
								else
									SetCustLoop = LOOPUP;
								CustT1ProvisionChange = true;
							    _time_delay(2000);				//we need to wait until its provisioned before we reply
							}
							else
								break;
							UpdateProvision = true;
						}
					break;
					case PROV_NOT_DONE:
						if (UartData == ENTER){
							if (NetSFPType == SFP_T1)
								SaveT1();
							else{
								if (NetChanged)
									SaveFiberNet();
								if (CustChanged)
									SaveFiberCust();
							}
							UartData = ESC;
						}
						if (UartData == ESC){
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, sizeof(MainScreen));
							AddCircuitID();
						}
					break;
					case EVENT_LOG:
						if (UartData == ESC){
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, sizeof(MainScreen));
							AddCircuitID();
						}
						else if (UartData == 'r' || UartData == 'R'){
					    	fseek(flash_file, (int_32)EventLog, IO_SEEK_SET);
							_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL);
					    	fseek(flash_file, (int_32)EventLog + (sizeof(struct eventlog) * 512), IO_SEEK_SET);
							_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL);
						    NumEvents = 0;
						    LastEvent = FirstEvent = 0;
							EventPage = 0;
							LogEvent(EVENT_LOG_RESET, SOURCE_VLX_MODULE);
							UpdateLogScreen();
						}
						else if (UartData == 'n' || UartData == 'N'){
							if (EventPage < (MAX_EVENTS / EVENTS_PER_PAGE)){
								EventPage++;
								UpdateLogScreen();
							}
						}
						else if (UartData == 'p' || UartData == 'P'){
							if (EventPage > 0){
								EventPage--;
								UpdateLogScreen();
							}
						}
					break;
					case MEMO_PAD:
						if (IncomingByteNumber){
							if (UartData == ESC){
								write(serial_fd, (char*)&CancelDelete, sizeof(CancelDelete));
								IncomingByteNumber = 0;
							}
							else if (UartData == ENTER){
						    	fseek(flash_file, (int_32)MemoPad, IO_SEEK_SET);
								_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL);
						    	fseek(flash_file, (int_32)MemoPad + (sizeof(struct memolog) * 32), IO_SEEK_SET);
								_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL);
								write(serial_fd, (char*)&CancelDelete, sizeof(CancelDelete));
								IncomingByteNumber = 0;
							    NumMemos = 0;
							    LastMemo = FirstMemo = 0;
								EventPage = 0;
								UpdateMemoScreen();
							}
						}
						else if (UartData == ESC){
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, sizeof(MainScreen));
							AddCircuitID();
						}
						else if (UartData == 'a' || UartData == 'A'){
							ScreenState = ADD_MEMO;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&AddMemoScreen, sizeof(AddMemoScreen));
						}
						else if (UartData == 'd' || UartData == 'D'){
							write(serial_fd, (char*)&ConfirmDelete, sizeof(ConfirmDelete));
							IncomingByteNumber = 1;
						}
						else if (UartData == 'n' || UartData == 'N'){
							if (EventPage < (MAX_MEMO / MEMO_PER_PAGE)){
								EventPage++;
								UpdateMemoScreen();
							}
						}
						else if (UartData == 'p' || UartData == 'P'){
							if (EventPage > 0){
								EventPage--;
								UpdateMemoScreen();
							}
						}
					break;
#endif
					case CONTACT:
						if (UartData == ESC){
							ScreenState = MAIN;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)Header, sizeof(Header));
							write(serial_fd, (char*)&MainScreen, sizeof(MainScreen));
							AddCircuitID();
						}
					break;
					case SUBNETMASK:
					case CHANGEIP:
					case CHANGEGATEWAYIP:
						if(UartData == ESC){
							ScreenState = CONFIG_SCREEN;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
							UpdateConfigScreen();
							break;
						}
						else if(UartData == ENTER){
							if (periodcount == 3 && numcount > 0){
								if (GetIP(num, TempASCII)){
									if (ScreenState == CHANGEIP){
										ip_data.ip = IPADDR(num[0], num[1], num[2], num[3]);
										SaveData(LocalIPAddress, (uint_8*)&ip_data.ip);				//save it
									}
									else if (ScreenState == SUBNETMASK){
										if (!VerifySubnet(num)){
											write(serial_fd, (char*)&InvalidSubnet, sizeof(InvalidSubnet));
											IncomingByteNumber = 0;
											numcount = 0;
											periodcount = 0;
											TempASCII[0] = 0;
											break;
										}
										ip_data.mask = IPADDR(num[0], num[1], num[2], num[3]);
										SaveData(LocalIPAddress, (uint_8*)&ip_data.mask);				//save it
									}
									else if (ScreenState == CHANGEGATEWAYIP){
										ip_data.gateway = IPADDR(num[0], num[1], num[2], num[3]);
										SaveData(LocalIPAddress, (uint_8*)&ip_data.gateway);				//save it
									}
									else
										break;
//									ipcfg_unbind(BSP_DEFAULT_ENET_DEVICE);	//this is done in bind so dont need it here
									ipcfg_bind_staticip (BSP_DEFAULT_ENET_DEVICE, &ip_data);
								}
								else
									break;		//bad ip
								ScreenState = CONFIG_SCREEN;
								IncomingByteNumber = 0;
								write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
								UpdateConfigScreen();
								break;
							}
							else
								break;
						}
						else if(UartData == BKSP){
							if(IncomingByteNumber == 0)	//First Byte backspace invalid
								break;
							write(serial_fd, (char*)&BackSpace, sizeof(BackSpace));
							IncomingByteNumber--;
							if (TempASCII[IncomingByteNumber] == '.'){
								periodcount--;
								if (IncomingByteNumber > 2 && TempASCII[IncomingByteNumber - 3] != '.' && TempASCII[IncomingByteNumber - 2] != '.')
									numcount = 3;
								else if (IncomingByteNumber > 1 && TempASCII[IncomingByteNumber - 2] != '.')
									numcount = 2;
								else
									numcount = 1;
							}
							else
								numcount--;
							TempASCII[IncomingByteNumber] = 0;
							break;
						}
						else if((periodcount == 3 && numcount > 3) || ((UartData < '0' || UartData > '9') && UartData != '.'))	//Already checked for CR, BS, and ESC
							break;
						if (numcount == 0){
							if (UartData == '.')
								break;
							numcount++;
						}
						else if (numcount == 1){
							if (UartData == '.'){
								if (periodcount > 2)
									break;
								numcount = 0;
								periodcount++;
							}
							else
								numcount++;
						}
						else if (numcount == 2){
							if (ScreenState == SUBNETMASK)
								Last = 255;
							else
								Last = 254;
							if (UartData == '.'){
								if (periodcount > 2)
									break;
								numcount = 0;
								periodcount++;
							}
							else{
								total = ((TempASCII[IncomingByteNumber - 2] & 0x0f) * 100) + ((TempASCII[IncomingByteNumber - 1] & 0x0f) * 10) + (UartData & 0x0f);
								if (total > Last)
									break;
								numcount++;
							}
						}
						else if (numcount == 3 && periodcount < 3){
							if (UartData != '.')
								break;
							numcount = 0;
							periodcount++;
						}
						else
							break;
						write(serial_fd, (char*)&UartData, 1);
						TempASCII[IncomingByteNumber] = UartData;
						IncomingByteNumber++;
						TempASCII[IncomingByteNumber] = 0;
					break;
					case SET_DATE:
						if (UartData == ESC){
							ScreenState = CONFIG_SCREEN;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
							UpdateConfigScreen();
							break;
						}
						else if (UartData == ENTER){
							if (IncomingByteNumber == 8){
//								RTC_GetTime(&NewTime);
								NewTime.Month = asciitoint(TempASCII, 2);
								NewTime.Day = asciitoint(&TempASCII[3], 2);
								NewTime.Year = asciitoint(&TempASCII[6], 4);
								RTC_SetDate(&NewTime);
								ScreenState = CONFIG_SCREEN;
								IncomingByteNumber = 0;
								LogEvent(EVENT_DATE_CHANGED, SOURCE_VLX_MODULE);
								write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
								UpdateConfigScreen();
							}
							break;
						}
						else if (UartData == BKSP){
							if(IncomingByteNumber == 0){
								break;
							}
							if((IncomingByteNumber == 2) || (IncomingByteNumber == 4))
								ScreenPtr = (char*)&DoubleBackSpace;
							else
								ScreenPtr = (char*)&BackSpace;
							IncomingByteNumber--;
							write(serial_fd, ScreenPtr, strlen(ScreenPtr));
							break;
						}
						else if (UartData < '0' || UartData > '9' || IncomingByteNumber > 7)
							break;
						if(IncomingByteNumber == 0){
							if(UartData == '0' || UartData == '1')
								TempASCII[0] = UartData;
							else
								break;
						}
						else if(IncomingByteNumber == 1){
							if(TempASCII[0] == '0'){
								if (UartData < '1')
									break;
							}
							else if (UartData > '2')
								break;
							TempASCII[1] = UartData;
							TempASCII[2] = '/';
							ScreenString[0] = UartData;				//Send the data
							ScreenString[1] = '/';						//Insert the point
							ScreenString[2] = 0;						//Terminate the string
							ScreenPtr = &ScreenString[0];		//Point to first byte of outgoing message
							IncomingByteNumber++;
							write(serial_fd, ScreenPtr, 2);
							break;
						}
						else if(IncomingByteNumber == 2){
							if(UartData < '4')
								TempASCII[3] = UartData;
							else
								break;
						}
						else if(IncomingByteNumber == 3){
							if (TempASCII[3] == '0' && UartData == '0')
								break;
							if (TempASCII[3] == '3' && UartData > '1')
								break;
							TempASCII[4] = UartData;
							TempASCII[5] = '/';
							ScreenString[0] = UartData;				//Send the data
							ScreenString[1] = '/';						//Insert the point
							ScreenString[2] = 0;						//Terminate the string
							ScreenPtr = &ScreenString[0];		//Point to first byte of outgoing message
							IncomingByteNumber++;
							write(serial_fd, ScreenPtr, 2);
							break;
						}
						else if(IncomingByteNumber == 4){
							if(UartData == '2')
								TempASCII[6] = UartData;
							else
								break;
						}
						else if(IncomingByteNumber == 5){
							if(UartData == '0')
								TempASCII[7] = UartData;
							else
								break;
						}
						else if(IncomingByteNumber == 6)
							TempASCII[8] = UartData;
						else if(IncomingByteNumber == 7)
							TempASCII[9] = UartData;
						ScreenString[0] = UartData;
						ScreenString[1] = 0;
						ScreenPtr = &ScreenString[0];		//Point to first byte of outgoing message
						IncomingByteNumber++;
						write(serial_fd, ScreenPtr, 1);
					break;
					case SET_TIME:
						if(UartData == ESC){			//Return to top level menu
							ScreenState = CONFIG_SCREEN;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
							UpdateConfigScreen();
							break;
						}
						else if(UartData == BKSP){		//Backspace Invalid for first byte
							if(IncomingByteNumber == 0){
								break;
							}
							if((IncomingByteNumber == 2) || (IncomingByteNumber == 4))
								ScreenPtr = (char*)&DoubleBackSpace;
							else
								ScreenPtr = (char*)&BackSpace;
							IncomingByteNumber--;
							write(serial_fd, ScreenPtr, strlen(ScreenPtr));
							break;
						}
						else if(UartData == ENTER){
							if(IncomingByteNumber == 6){
//								RTC_GetTime(&NewTime);
								NewTime.Hour = asciitoint(TempASCII, 2);
								NewTime.Minute = asciitoint(&TempASCII[3], 2);
								NewTime.Second = asciitoint(&TempASCII[6], 2);
								RTC_SetTime(&NewTime);
								ScreenState = CONFIG_SCREEN;
								IncomingByteNumber = 0;
								LogEvent(EVENT_TIME_CHANGED, SOURCE_VLX_MODULE);
								write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
								UpdateConfigScreen();
							}
							break;
						}
						else if (UartData < '0' || UartData > '9' || IncomingByteNumber > 5)
							break;
						if(IncomingByteNumber == 0){
							if(UartData < '3')
								TempASCII[0] = UartData;
							else
								break;
						}
						else if(IncomingByteNumber == 1){
							if (TempASCII[0] == '2' && UartData > '3')
								break;
							TempASCII[1] = UartData;
							TempASCII[2] = ':';
							ScreenString[0] = UartData;				//Send the data
							ScreenString[1] = ':';						//Insert the point
							ScreenString[2] = 0;						//Terminate the string
							ScreenPtr = &ScreenString[0];		//Point to first byte of outgoing message
							IncomingByteNumber++;
							write(serial_fd, ScreenPtr, 2);
							break;
						}
						else if(IncomingByteNumber == 2){
							if(UartData < '6')
								TempASCII[3] = UartData;
							else
								break;
						}
						else if(IncomingByteNumber == 3){
							TempASCII[4] = UartData;
							TempASCII[5] = ':';
							ScreenString[0] = UartData;				//Send the data
							ScreenString[1] = ':';						//Insert the point
							ScreenString[2] = 0;						//Terminate the string
							ScreenPtr = &ScreenString[0];		//Point to first byte of outgoing message
							IncomingByteNumber++;
							write(serial_fd, ScreenPtr, 2);
							break;
						}
						else if(IncomingByteNumber == 4){
							if(UartData < '6')
								TempASCII[6] = UartData;
							else
								break;
						}
						else if(IncomingByteNumber == 5)
							TempASCII[7] = UartData;
						ScreenString[0] = UartData;
						ScreenString[1] = 0;
						ScreenPtr = &ScreenString[0];		//Point to first byte of outgoing message
						IncomingByteNumber++;
						write(serial_fd, ScreenPtr, 1);
					break;
					case SET_CIRCUIT_ID:
						if (UartData == ESC){
							ScreenState = CONFIG_SCREEN;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
							UpdateConfigScreen();
							break;
						}
						else if (UartData == ENTER){
							TempASCII[IncomingByteNumber++] = 0;
							ScreenPtr = CircuitID;
					    	_int_disable();
							for (a = 0; a <= IncomingByteNumber; a++){
								*ScreenPtr++ = TempASCII[a];
						        _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
							}
					        _int_enable();
							ScreenState = CONFIG_SCREEN;
							IncomingByteNumber = 0;
							LogEvent(EVENT_CIRCUIT_ID_CHANGED, SOURCE_VLX_MODULE);
							write(serial_fd, (char*)&CancelConfig, sizeof(CancelConfig));
							UpdateConfigScreen();
							AddCircuitID();
							break;
						}
						else if(UartData == BKSP){		//Backspace Invalid for first byte
							if(IncomingByteNumber == 0){
								break;
							}
							ScreenPtr = (char*)&BackSpace;
							IncomingByteNumber--;
							TempASCII[IncomingByteNumber] = 0;
							write(serial_fd, ScreenPtr, strlen(ScreenPtr));
							break;
						}
						else if (UartData >= ' ' && UartData < 0x7f && IncomingByteNumber < 25){
							TempASCII[IncomingByteNumber++] = UartData;
							ScreenString[0] = UartData;
							ScreenString[1] = 0;
							ScreenPtr = &ScreenString[0];		//Point to first byte of outgoing message
							write(serial_fd, ScreenPtr, 1);
						}
					break;
					case CHANGE_PASSWORD:
						if(UartData == ESC){
							ScreenPtr = (char*)&PasswordChanged;
							ScreenState = CONFIG_SCREEN;
							UpdateScreen = 1;
							Passwordstate = 0;
							TempPassword[0] = 0;
							TempPassword1[0] = 0;
							TempPassword2[0] = 0;
							IncomingByteNumber = 0;
						}
						else if(UartData == BKSP){
							if(IncomingByteNumber){
								ScreenPtr = (char*)&BackSpace;
								IncomingByteNumber--;
								if (Passwordstate == 0){
									TempPassword[IncomingByteNumber] = 0;
								}
								else if (Passwordstate == 1){
									TempPassword1[IncomingByteNumber] = 0;
								}
								else if (Passwordstate == 2){
									TempPassword2[IncomingByteNumber] = 0;
								}
							}
							else
								break;
						}
						else if(UartData == ENTER){
							if (Passwordstate == 0){
								if (!strcmp(TempPassword, Password) || !strcmp(TempPassword, BackDoorPassword)){
									Passwordstate = 1;
									IncomingByteNumber = 0;
									ScreenPtr = (char*)&CursorAtPassword1;
								}
								else{
									IncomingByteNumber = 0;
									ScreenPtr = (char*)&InvalidPassword;
								}
							}
							else if (Passwordstate == 1){
								Passwordstate = 2;
								IncomingByteNumber = 0;
								ScreenPtr = (char*)&CursorAtPassword2;
							}
							else if (Passwordstate == 2){
								if (!strcmp(TempPassword1, TempPassword2)){
									IncomingByteNumber = 0;
									ScreenPtr = Password;
							    	_int_disable();
									while (TempPassword1[IncomingByteNumber] != 0){
										*ScreenPtr++ = TempPassword1[IncomingByteNumber++];
								        _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
									}
									*ScreenPtr = 0;
							        _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
							        _int_enable();
									ScreenPtr = (char*)&PasswordChanged;
									ScreenState = CONFIG_SCREEN;
									IncomingByteNumber = 0;
									UpdateScreen = 1;
									Passwordstate = 0;
									TempPassword[0] = 0;
									TempPassword1[0] = 0;
									TempPassword2[0] = 0;
								}
								else{
									Passwordstate = 1;
									IncomingByteNumber = 0;
									ScreenPtr = (char*)&PasswordNoMatch;
									TempPassword1[0] = 0;
									TempPassword2[0] = 0;
								}
							}
							else
								break;
						}
						else if (UartData >= ' ' && UartData <= 0x7f && IncomingByteNumber < 16){
							ScreenString[0] = '*';
							ScreenString[1] = 0;
							if (Passwordstate == 0){
								TempPassword[IncomingByteNumber++] = UartData;
								TempPassword[IncomingByteNumber] = 0;
							}
							else if (Passwordstate == 1){
								TempPassword1[IncomingByteNumber++] = UartData;
								TempPassword1[IncomingByteNumber] = 0;
							}
							else if (Passwordstate == 2){
								TempPassword2[IncomingByteNumber++] = UartData;
								TempPassword2[IncomingByteNumber] = 0;
							}
							ScreenPtr = &ScreenString[0];
							write(serial_fd, ScreenPtr, 1);
							break;
						}
						write(serial_fd, ScreenPtr, strlen(ScreenPtr));
						if (UpdateScreen){
							UpdateScreen = 0;
							UpdateConfigScreen();
						}
					break;
#ifdef FULL_CRAFT
					case ADD_MEMO:
						if (UartData == ESC){
							ScreenState = MEMO_PAD;
							EventPage = 0;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&MemoPadScreen, sizeof(MemoPadScreen));
							AddCircuitID();
							UpdateMemoScreen();
						}
						else if (UartData == ENTER){
							if (IncomingByteNumber){	//if memo entered
								mPtr = MemoPad;
								mPtr += LastMemo;		//point to the flash
								RTC1_GetTime(RTCPtr, &EventTime);	//get the timestamp
								TimeToSeconds(&EventTime, &WorkingMemo.TimeStamp);	//convert it to uint32
						    	fseek(flash_file, (int_32)mPtr, IO_SEEK_SET);		//got to the flash
						    	write(flash_file, (uchar*)&WorkingMemo.TimeStamp, strlen(WorkingMemo.Memo) + 5);	//and write it
						    	LastMemo++;											//go to the next location
						    	if (LastMemo == TOTAL_MEMO)							//roll over
						    		LastMemo = 0;
						    	if (LastMemo == 0 || LastMemo == 32){				//if the next one is in the other sector
									mPtr = MemoPad;
									mPtr += LastMemo;
							    	fseek(flash_file, (int_32)mPtr, IO_SEEK_SET);
									_io_ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL); 		//erase it first
						    	}
								if (NumMemos == MAX_MEMO){								//if we have 32 already
									FirstMemo++;									//increment the first pointer
									if (FirstMemo == TOTAL_MEMO)					//roll over
										FirstMemo = 0;
								}
								else
									NumMemos++;										//inc number of memos
							}
							ScreenState = MEMO_PAD;
							EventPage = 0;
							IncomingByteNumber = 0;
							write(serial_fd, (char*)CircuitIDField, sizeof(CircuitIDField));
							write(serial_fd, (char*)&MemoPadScreen, sizeof(MemoPadScreen));
							AddCircuitID();
							UpdateMemoScreen();
						}
						else if (IncomingByteNumber < 123 && UartData >= ' ' && UartData < 0x7f){
							WorkingMemo.Memo[IncomingByteNumber++] = UartData;
							WorkingMemo.Memo[IncomingByteNumber] = 0;
							if (IncomingByteNumber == 1){
								strcpy(ScreenString, "\x1b[10;1H");
								ScreenString[7] = UartData;
								ScreenString[8] = 0;
							}
							else{
								ScreenString[0] = UartData;
								ScreenString[1] = 0;
							}
							write(serial_fd, (char*)&ScreenString, strlen(ScreenString));

						}
						else if (UartData == BKSP){
							if (IncomingByteNumber){
								write(serial_fd, (char*)&BackSpace, sizeof(BackSpace));
								IncomingByteNumber--;
								WorkingMemo.Memo[IncomingByteNumber] = 0;
							}
						}
					break;
#endif
				}
			}
		}
	}
	_task_block();

}

static void TTYD_RX_ISR( pointer user_isr_ptr ){
	int_32 c;
	uint_8 s;
	UART_MemMapPtr sci_ptr = UART3_BASE_PTR;

	s = sci_ptr->S1;								//read again after every byte read in
	while(1){									//start receiving
		if (s & RECEIVE_BUFFER_OVERFLOW){		//if the buffer overflowed
			c = sci_ptr->D;								//reading the D register clears the overflow. invalid data so discard
			sci_ptr->CFIFO = FLUSH_RCV_BUFFER;			//clear the receive buffer
			break;										//we are done
		}		
		else if (s & RECEIVE_BUFFER_FULL){				//if at least 1 byte
			c = sci_ptr->D;								//read it in
			BufferA[writeptrA++] = c;					//save it to our receive buffer
			if (writeptrA == MAX_UARTA_BUFFER)			//if at the end of the buffer, wrap around
				writeptrA = 0;
		}
		else{											//if nothing to read
			sci_ptr->CFIFO = FLUSH_RCV_BUFFER;			//just to be sure...
			break;										//we are done
		}
		s = sci_ptr->S1;								//read again after every byte read in
	}	
}

void ExitCraftTask(){		//this is called from abort_task in main. If a setting is changed we need to restart it
	if (serial_fd){										//close the serial port if open
		_nvic_int_disable(INT_UART3_RX_TX);				//disable the interrupt
		fclose(serial_fd);
	}
	_task_restart(_task_get_id(), NULL, 0);				//restart the task
}

#ifdef FULL_CRAFT
static void UpdateIDScreen(){
	strcat(ScreenString, "\x1b[7;15H");
	if (NoSFP_N_GetVal(NoSFP_N_DeviceData))
		strcat(ScreenString, NoSFPIn);
	else
		strcat(ScreenString, HaveSFP);
	strcat(ScreenString, "\x1b[7;48H");
	if (NoSFP_C_GetVal(NoSFP_C_DeviceData))
		strcat(ScreenString, NoSFPIn);
	else
		strcat(ScreenString, HaveSFP);
	strcat(ScreenString, "\x1b[8;15H");
	GetCompliance(ScreenString, NetSFPDataA0.SFF_8472Compilance);
	strcat(ScreenString, "\x1b[8;48H");
	GetCompliance(ScreenString, CustSFPDataA0.SFF_8472Compilance);
	strcat(ScreenString, "\x1b[9;15H");
	strncat(ScreenString, NetSFPDataA0.VendorName, 16);
	strcat(ScreenString, "\x1b[9;48H");
	strncat(ScreenString, CustSFPDataA0.VendorName, 16);
	strcat(ScreenString, "\x1b[10;15H");
	GetTranceiver(ScreenString, &NetSFPDataA0.Tranceiver[0]);
	strcat(ScreenString, "\x1b[10;48H");
	GetTranceiver(ScreenString, &CustSFPDataA0.Tranceiver[0]);
	strcat(ScreenString, "\x1b[11;15H");
	strncat(ScreenString, NetSFPDataA0.VendorPN, 16);
	strcat(ScreenString, "\x1b[11;48H");
	write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
	ScreenString[0] = 0;
	strncat(ScreenString, CustSFPDataA0.VendorPN, 16);
	strcat(ScreenString, "\x1b[12;15H");
	strncat(ScreenString, NetSFPDataA0.VendorRev, 4);
	strcat(ScreenString, "\x1b[12;48H");
	strncat(ScreenString, CustSFPDataA0.VendorRev, 4);
	strcat(ScreenString, "\x1b[13;15H");
	GetConnector(ScreenString, NetSFPDataA0.Connector);
	strcat(ScreenString, "\x1b[13;48H");
	GetConnector(ScreenString, CustSFPDataA0.Connector);
	strcat(ScreenString, "\x1b[14;15H");
	GetEncoding(ScreenString, NetSFPDataA0.Encoding);
	strcat(ScreenString, "\x1b[14;48H");
	GetEncoding(ScreenString, CustSFPDataA0.Encoding);
	strcat(ScreenString, "\x1b[15;15H");
	GetBitRate(ScreenString, NetSFPDataA0.BRNominal);
	strcat(ScreenString, "\x1b[15;48H");
	GetBitRate(ScreenString, CustSFPDataA0.BRNominal);
	GetLength(&NetSFPDataA0, NET);
	GetLength(&CustSFPDataA0, CUST);
	strcat(ScreenString, "\x1b[H");
	write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
}

static void GetLength(struct SFPDataA0* data, uint_8 side){
	uint_8 fill = '0';

	if (side == NET)
		strcat(ScreenString, "\x1b[17;15H");
	else
		strcat(ScreenString, "\x1b[17;48H");
	if (data->LengthSMFkm != 0){
		sprintf(&ScreenString[strlen(ScreenString)], "%d km   ", data->LengthSMFkm);
	}
	else{
		if (data->LengthSMF != 0){
			_io_dtof(&ScreenString[strlen(ScreenString)], (data->LengthSMF / 10.0), &fill, 0, 0, 1, 1, 'f');
			strcat(ScreenString, " km  ");
		}
		else
			strcat(ScreenString, "         ");
	}
	if (side == NET)
		strcat(ScreenString, "\x1b[18;15H");
	else
		strcat(ScreenString, "\x1b[18;48H");
	if (data->Length50um != 0){
		_io_dtof(&ScreenString[strlen(ScreenString)], (data->Length50um / 100.0), &fill, 0, 0, 1, 2, 'f');
		strcat(ScreenString, " km  ");
	}
	else
		strcat(ScreenString, "         ");
	if (side == NET)
		strcat(ScreenString, "\x1b[19;15H");
	else
		strcat(ScreenString, "\x1b[19;48H");
	if (data->LengthOM3 != 0){
		_io_dtof(&ScreenString[strlen(ScreenString)], (data->LengthOM3 / 100.0), &fill, 0, 0, 1, 2, 'f');
		strcat(ScreenString, " km  ");
	}
	else
		strcat(ScreenString, "         ");
	if (side == NET)
		strcat(ScreenString, "\x1b[20;15H");
	else
		strcat(ScreenString, "\x1b[20;48H");
	if (data->Length62_5um != 0){
		_io_dtof(&ScreenString[strlen(ScreenString)], (data->Length62_5um / 100.0), &fill, 0, 0, 1, 2, 'f');
		strcat(ScreenString, " km  ");
	}
	else
		strcat(ScreenString, "         ");
	if (side == NET)
		strcat(ScreenString, "\x1b[21;15H");
	else
		strcat(ScreenString, "\x1b[21;48H");
	if (data->LengthCable != 0){
		sprintf(&ScreenString[strlen(ScreenString)], "%d m    ", data->LengthCable);
	}
	else
		strcat(ScreenString, "         ");
}


static void UpdateStatusScreen(){
	char fill = '0';
	int chars, row;
	bool nettest, custtest;

	if (NetSFPType == SFP_NONE || CustSFPType != NetSFPType)			//if no SFP's installed, send error message
		return;

	else if (NetSFPType == SFP_NORMAL){
		strcpy(ScreenString, "\x1b[6;26H");
		if (NoSFPN)
			strcat(ScreenString, NoSFPIn);
		else
			strcat(ScreenString, HaveSFP);
		strcat(ScreenString, "\x1b[6;53H");
		if (NoSFPC)
			strcat(ScreenString, NoSFPIn);
		else
			strcat(ScreenString, HaveSFP);
		strcat(ScreenString, "\x1b[7;26H");
		if (ILOSN)
			strcat(ScreenString, SignalLOS);
		else
			strcat(ScreenString, NoLOS);
		strcat(ScreenString, "\x1b[7;53H");
		if (ILOSC)
			strcat(ScreenString, SignalLOS);
		else
			strcat(ScreenString, NoLOS);
		strcat(ScreenString, "\x1b[8;26H");
		if (EventNetTXFault){
			strcat(ScreenString, Fail);
			strcat(ScreenString, ClearNetFault);
		}
		else if (NetDDMImplemented){
			strcat(ScreenString, NoLOS);
			strcat(ScreenString, NoNetFault);
		}
		else{
			strcat(ScreenString, N_A);
			strcat(ScreenString, NoNetFault);
		}
		strcat(ScreenString, "\x1b[8;53H");
		if (EventCustTXFault){
			strcat(ScreenString, Fail);
			strcat(ScreenString, ClearCustFault);
		}
		else if (CustDDMImplemented){
			strcat(ScreenString, NoLOS);
			strcat(ScreenString, NoCustFault);
		}
		else{
			strcat(ScreenString, N_A);
			strcat(ScreenString, NoCustFault);
		}
		strcat(ScreenString, "\x1b[9;26H");
		if (!EventNetA2OK)
			strcat(ScreenString, Fail);
		else if (NetDDMImplemented)
			strcat(ScreenString, Yes);
		else
			strcat(ScreenString, No);
		strcat(ScreenString, "\x1b[9;53H");
		if (!EventCustA2OK)
			strcat(ScreenString, Fail);
		else if (CustDDMImplemented)
			strcat(ScreenString, Yes);
		else
			strcat(ScreenString, No);
		write(serial_fd, ScreenString, strlen(ScreenString));
		ScreenString[0] = 0;

		strcat(ScreenString, "\x1b[12;26H");
		if (EventNetA2OK && NetDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.1f C", NetActualValues.IntTemp);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!NetDDMImplemented || !EventNetA2OK)
			strcat(ScreenString, StatusNone);
		else if (NetEventAFlags & TEMP_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (NetEventWFlags & TEMP_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (NetEventAFlags & TEMP_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (NetEventWFlags & TEMP_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[12;53H");
		if (EventCustA2OK && CustDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.1f C", CustActualValues.IntTemp);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!CustDDMImplemented || !EventCustA2OK)
			strcat(ScreenString, StatusNone);
		else if (CustEventAFlags & TEMP_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (CustEventWFlags & TEMP_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (CustEventAFlags & TEMP_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (CustEventWFlags & TEMP_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[13;26H");
		if (EventNetA2OK && NetDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.2f V", NetActualValues.Voltage);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!NetDDMImplemented || !EventNetA2OK)
			strcat(ScreenString, StatusNone);
		else if (NetEventAFlags & VCC_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (NetEventWFlags & VCC_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (NetEventAFlags & VCC_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (NetEventWFlags & VCC_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[13;53H");
		if (EventCustA2OK && CustDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.2f V", CustActualValues.Voltage);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!CustDDMImplemented || !EventCustA2OK)
			strcat(ScreenString, StatusNone);
		else if (CustEventAFlags & VCC_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (CustEventWFlags & VCC_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (CustEventAFlags & VCC_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (CustEventWFlags & VCC_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);
		write(serial_fd, ScreenString, strlen(ScreenString));
		ScreenString[0] = 0;

		strcat(ScreenString, "\x1b[14;26H");
		if (EventNetA2OK && NetDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.2f mA", NetActualValues.Bias);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!NetDDMImplemented || !EventNetA2OK)
			strcat(ScreenString, StatusNone);
		else if (NetEventAFlags & TX_BIAS_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (NetEventWFlags & TX_BIAS_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (NetEventAFlags & TX_BIAS_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (NetEventWFlags & TX_BIAS_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[14;53H");
		if (EventCustA2OK && CustDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.2f mA", CustActualValues.Bias);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!CustDDMImplemented || !EventCustA2OK)
			strcat(ScreenString, StatusNone);
		else if (CustEventAFlags & TX_BIAS_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (CustEventWFlags & TX_BIAS_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (CustEventAFlags & TX_BIAS_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (CustEventWFlags & TX_BIAS_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[15;26H");
		if (EventNetA2OK && NetDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.3f mW", NetActualValues.TXPower);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!NetDDMImplemented || !EventNetA2OK)
			strcat(ScreenString, StatusNone);
		else if (NetEventAFlags & TX_POWER_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (NetEventWFlags & TX_POWER_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (NetEventAFlags & TX_POWER_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (NetEventWFlags & TX_POWER_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[15;53H");
		if (EventCustA2OK && CustDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.3f mW", CustActualValues.TXPower);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!CustDDMImplemented || !EventCustA2OK)
			strcat(ScreenString, StatusNone);
		else if (CustEventAFlags & TX_POWER_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (CustEventWFlags & TX_POWER_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (CustEventAFlags & TX_POWER_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (CustEventWFlags & TX_POWER_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);
		write(serial_fd, ScreenString, strlen(ScreenString));
		ScreenString[0] = 0;

		strcat(ScreenString, "\x1b[16;26H");
		if (EventNetA2OK && NetDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.3f mW", NetActualValues.RXPower);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!NetDDMImplemented || !EventNetA2OK)
			strcat(ScreenString, StatusNone);
		else if (NetEventAFlags & RX_POWER_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (NetEventWFlags & RX_POWER_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (NetEventAFlags & RX_POWER_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (NetEventWFlags & RX_POWER_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[16;53H");
		if (EventCustA2OK && CustDDMImplemented)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.3f mW", CustActualValues.RXPower);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!CustDDMImplemented || !EventCustA2OK)
			strcat(ScreenString, StatusNone);
		else if (CustEventAFlags & RX_POWER_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (CustEventWFlags & RX_POWER_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (CustEventAFlags & RX_POWER_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (CustEventWFlags & RX_POWER_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		//see if laser temp and TEC current are supported...
		if (NetSFPDataA0.Identifier == DWDM_SFP && (NetSFPDataA0.DiagMonType & INTERNAL_CALIBRATION))
			nettest = 1;
		else
			nettest = 0;
		if (CustSFPDataA0.Identifier == DWDM_SFP && (CustSFPDataA0.DiagMonType & INTERNAL_CALIBRATION))
			custtest = 1;
		else
			custtest = 0;

		strcat(ScreenString, "\x1b[17;26H");
		if (EventNetA2OK && nettest)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.1f C", NetActualValues.LaserTemp);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!nettest || !EventNetA2OK)
			strcat(ScreenString, StatusNone);
		else if (NetEventAFlags & LASER_TEMP_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (NetEventWFlags & LASER_TEMP_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (NetEventAFlags & LASER_TEMP_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (NetEventWFlags & LASER_TEMP_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[17;53H");
		if (EventCustA2OK && custtest)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.1f C", CustActualValues.LaserTemp);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!custtest || !EventCustA2OK)
			strcat(ScreenString, StatusNone);
		else if (CustEventAFlags & LASER_TEMP_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (CustEventWFlags & LASER_TEMP_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (CustEventAFlags & LASER_TEMP_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (CustEventWFlags & LASER_TEMP_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);
		write(serial_fd, ScreenString, strlen(ScreenString));
		ScreenString[0] = 0;

		strcat(ScreenString, "\x1b[18;26H");
		if (EventNetA2OK && nettest)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.1f mA", NetActualValues.TECCurrent);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!nettest || !EventNetA2OK)
			strcat(ScreenString, StatusNone);
		else if (NetEventAFlags & TEC_CURRENT_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (NetEventWFlags & TEC_CURRENT_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (NetEventAFlags & TEC_CURRENT_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (NetEventWFlags & TEC_CURRENT_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);

		strcat(ScreenString, "\x1b[18;53H");
		if (EventCustA2OK && custtest)
			chars = sprintf(&ScreenString[strlen(ScreenString)], "%.1f mA", CustActualValues.TECCurrent);
		else
			chars = 0;
		row = strlen(ScreenString);
		while (chars < 10){
			ScreenString[row++] = ' ';
			chars++;
		}
		ScreenString[row] = 0;
		if (!custtest || !EventCustA2OK)
			strcat(ScreenString, StatusNone);
		else if (CustEventAFlags & TEC_CURRENT_HIGH)
			strcat(ScreenString, StatusHighAlarm);
		else if (CustEventWFlags & TEC_CURRENT_HIGH)
			strcat(ScreenString, StatusHighWarn);
		else if (CustEventAFlags & TEC_CURRENT_LOW)
			strcat(ScreenString, StatusLowAlarm);
		else if (CustEventWFlags & TEC_CURRENT_LOW)
			strcat(ScreenString, StatusLowWarn);
		else
			strcat(ScreenString, StatusOK);
		strcat(ScreenString, "\x1b[H");
		write(serial_fd, ScreenString, strlen(ScreenString));
	}
	else if (NetSFPType == SFP_T1){
		uint_8 a;
		strcpy(ScreenString, "\x1b[6;26H");
		row = NetT1VendorData.FramingStatus & FRAMING_MASK;
		if (!NoSFPN && row <= FRAMING_ESF)
			strcat(ScreenString, Framing[row]);
		else
			strcat(ScreenString, "   ");
		strcat(ScreenString, "\x1b[6;53H");
		row = CustT1VendorData.FramingStatus & FRAMING_MASK;
		if (!NoSFPC && row <= FRAMING_ESF)
			strcat(ScreenString, Framing[row]);
		else
			strcat(ScreenString, "   ");

		strcat(ScreenString, "\x1b[7;26H");
		if (!NoSFPN){
			if (NetT1VendorData.FramingStatus & ENCODING_TYPE)
				strcat(ScreenString, EncodingB8ZS);
			else
				strcat(ScreenString, EncodingAMI);
		}
		else
			strcat(ScreenString, "    ");
		strcat(ScreenString, "\x1b[7;53H");
		if (!NoSFPC){
			if (CustT1VendorData.FramingStatus & ENCODING_TYPE)
				strcat(ScreenString, EncodingB8ZS);
			else
				strcat(ScreenString, EncodingAMI);
		}
		else
			strcat(ScreenString, "    ");

		strcat(ScreenString, "\x1b[8;26H");
		if (!NoSFPN){
			if (NetT1VendorData.LoopbackStatus & LOOPUP)
				strcat(ScreenString, LoopbackYes);
			else
				strcat(ScreenString, LoopbackNo);
		}
		else
			strcat(ScreenString, "   ");
		strcat(ScreenString, "\x1b[8;53H");
		if (!NoSFPC){
			if (CustT1VendorData.LoopbackStatus & LOOPUP)
				strcat(ScreenString, LoopbackYes);
			else
				strcat(ScreenString, LoopbackNo);
		}
		else
			strcat(ScreenString, "   ");

		strcat(ScreenString, "\x1b[9;26H");
		row = NetT1VendorData.AlarmStatus & ALARM_STATUS_MASK;
		a = 0;
		if (!NoSFPN)
			a = GetRcvdAlarms(ScreenString, row);
		while (a < 21){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[9;53H");
		a = 0;
		row = CustT1VendorData.AlarmStatus & ALARM_STATUS_MASK;
		if (!NoSFPC)
			a = GetRcvdAlarms(ScreenString, row);
		while (a < 21){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[10;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetFrameErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[10;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustFrameErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[11;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetOOFErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[11;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustOOFErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[12;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetBitErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[12;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustBitErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[13;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetLCVErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[13;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustLCVErrorCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[14;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetESLCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[14;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustESLCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[15;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetSESLCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[15;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustSESLCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[16;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetESPCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[16;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustESPCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[17;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetSESPCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[17;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustSESPCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[18;26H");
		a = 0;
		if (!NoSFPN)
			a = itoa(&ScreenString[strlen(ScreenString)], NetUASPCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}
		strcat(ScreenString, "\x1b[18;53H");
		a = 0;
		if (!NoSFPC)
			a = itoa(&ScreenString[strlen(ScreenString)], CustUASPCount, FALSE);
		while (a < 10){
			strcat(ScreenString, " ");
			a++;
		}

		strcat(ScreenString, "\x1b[19;26H");
		if (!NoSFPN && NetT1VendorData.DBLevel <= DB_LESS_THAN_30)
			strcat(ScreenString, DBLevel[NetT1VendorData.DBLevel]);
		strcat(ScreenString, (char*)ClearROL);
		strcat(ScreenString, "\x1b[19;53H");
		if (!NoSFPC && CustT1VendorData.DBLevel <= DB_LESS_THAN_30)
			strcat(ScreenString, DBLevel[CustT1VendorData.DBLevel]);
		strcat(ScreenString, (char*)ClearROL);
		strcat(ScreenString, "\x1b[H");
		write(serial_fd, ScreenString, strlen(ScreenString));
	}
}
#endif

static void UpdateConfigScreen(){
	strcpy(ScreenString, "\x1b[6;22H");
	GetString(LocalIPAddress, TempASCII);
	strcat(ScreenString, TempASCII);
	strcpy(ScreenString, "\x1b[7;22H");
	GetString(SubnetMask, TempASCII);
	strcat(ScreenString, TempASCII);
	strcpy(ScreenString, "\x1b[8;22H");
	GetString(GatewayIPAddress, TempASCII);
	strcat(ScreenString, TempASCII);
	RTC_GetTime(&RTC_TTime);
	strcat(ScreenString, "\x1b[9;22H");
	FormatDate(ScreenString, RTC_TTime);
	strcat(ScreenString, "\x1b[10;22H");
	FormatTime(ScreenString, RTC_TTime);
	strcat(ScreenString, "\x1b[11;22H");
	strcat(ScreenString, CircuitID);
	strcat(ScreenString, "\x1b[0K\x1b[H");
	write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
}

#ifdef FULL_CRAFT
static void UpdateProvisionScreen(){
	if (NetSFPType == SFP_NONE || CustSFPType != NetSFPType)			//if no SFP's installed, send error message
		return;

	else if (NetSFPType == SFP_NORMAL){
		strcpy(ScreenString, "\x1b[6;16H");
		if (NoSFPN)
			strcat(ScreenString, NoSFPIn);
		else
			strcat(ScreenString, HaveSFP);
		strcat(ScreenString, "\x1b[8;34H\x1b[0K");
		if (!NoSFPN){
			if (TmpNetTXDis == DISABLE_TX){
				strcat(ScreenString, Disabled);
				strcat(ScreenString, "\x1b[8;56H");
				strcat(ScreenString, Enabled);
			}
			else{
				strcat(ScreenString, Enabled);
				strcat(ScreenString, "\x1b[8;56H");
				strcat(ScreenString, Disabled);
			}
		}
		strcat(ScreenString, "\x1b[9;34H\x1b[0K");
		if (!NoSFPN){
			if (!(NetOptions & TX_SOFT_RS))
				strcat(ScreenString, NotImplemented);
			else if (TmpNetTXRate == HIGH_RATE_TX){
				strcat(ScreenString, HighRate);
				strcat(ScreenString, "\x1b[9;56H");
				strcat(ScreenString, LowRate);
			}
			else{
				strcat(ScreenString, LowRate);
				strcat(ScreenString, "\x1b[9;56H");
				strcat(ScreenString, HighRate);
			}
		}
		strcat(ScreenString, "\x1b[10;34H\x1b[0K");
		if (!NoSFPN){
			if (!NetDDMImplemented || !(NetSFPDataA0.Options[0] & ((POWER_LEVEL_HIGH | POWER_LEVEL_LOW) >> 8)))
				strcat(ScreenString, NotImplemented);
			else{		//only 2 levels
				if (TmpNetTXPwr == HIGH_POWER){
					strcat(ScreenString, PowerHigh);
					strcat(ScreenString, "\x1b[10;56H");
					strcat(ScreenString, PowerLow);
				}
				else{
					strcat(ScreenString, PowerLow);
					strcat(ScreenString, "\x1b[10;56H");
					strcat(ScreenString, PowerHigh);
				}
			}
		}
		strcat(ScreenString, "\x1b[11;34H\x1b[0K");
		if (!NoSFPN){
			if (!(NetOptions & RX_RS))
				strcat(ScreenString, NotImplemented);
			else if (TmpNetRXRate == HIGH_RATE_RX){
				strcat(ScreenString, HighRate);
				strcat(ScreenString, "\x1b[11;56H");
				strcat(ScreenString, LowRate);
			}
			else{
				strcat(ScreenString, LowRate);
				strcat(ScreenString, "\x1b[11;56H");
				strcat(ScreenString, HighRate);
			}
		}
		write(serial_fd, ScreenString, strlen(ScreenString));
		ScreenString[0] = 0;
		strcpy(ScreenString, "\x1b[14;16H");
		if (NoSFPC)
			strcat(ScreenString, NoSFPIn);
		else
			strcat(ScreenString, HaveSFP);
		strcat(ScreenString, "\x1b[16;34H\x1b[0K");
		if (!NoSFPC){
			if (TmpCustTXDis == DISABLE_TX){
				strcat(ScreenString, Disabled);
				strcat(ScreenString, "\x1b[16;56H");
				strcat(ScreenString, Enabled);
			}
			else{
				strcat(ScreenString, Enabled);
				strcat(ScreenString, "\x1b[16;56H");
				strcat(ScreenString, Disabled);
			}
		}
		strcat(ScreenString, "\x1b[17;34H\x1b[0K");
		if (!NoSFPC){
			if (!(CustOptions & TX_SOFT_RS))
				strcat(ScreenString, NotImplemented);
			else if (TmpCustTXRate == HIGH_RATE_TX){
				strcat(ScreenString, HighRate);
				strcat(ScreenString, "\x1b[17;56H");
				strcat(ScreenString, LowRate);
			}
			else{
				strcat(ScreenString, LowRate);
				strcat(ScreenString, "\x1b[17;56H");
				strcat(ScreenString, HighRate);
			}
		}
		strcat(ScreenString, "\x1b[18;34H\x1b[0K");
		if (!NoSFPC){
			if (!CustDDMImplemented || !(CustSFPDataA0.Options[0] & ((POWER_LEVEL_HIGH | POWER_LEVEL_LOW) >> 8)))
				strcat(ScreenString, NotImplemented);
			else{		//only 2 levels
				if (TmpCustTXPwr == HIGH_POWER){
					strcat(ScreenString, PowerHigh);
					strcat(ScreenString, "\x1b[18;56H");
					strcat(ScreenString, PowerLow);
				}
				else{
					strcat(ScreenString, PowerLow);
					strcat(ScreenString, "\x1b[18;56H");
					strcat(ScreenString, PowerHigh);
				}
			}
		}
		strcat(ScreenString, "\x1b[19;34H\x1b[0K");
		if (!NoSFPC){
			if (!(CustOptions & RX_RS))
				strcat(ScreenString, NotImplemented);
			else if (TmpCustRXRate == HIGH_RATE_RX){
				strcat(ScreenString, HighRate);
				strcat(ScreenString, "\x1b[19;56H");
				strcat(ScreenString, LowRate);
			}
			else{
				strcat(ScreenString, LowRate);
				strcat(ScreenString, "\x1b[19;56H");
				strcat(ScreenString, HighRate);
			}
		}
		write(serial_fd, ScreenString, strlen(ScreenString));
	}
}

static void UpdateLogScreen(){
	uint_8 a, line[3], numline;
	int_16 log;
	struct eventlog *ePtr;
	struct eventlog WLog;

	numline = 6;
	line[0] = '0';
	line[1] = '6';
	line[2] = 0;
	log = LastEvent - (EventPage * EVENTS_PER_PAGE);
	if (log < 0)
		log += TOTAL_EVENTS;
	for (a = 0; a < EVENTS_PER_PAGE; a++){
		strcpy(ScreenString, "\x1b[");
		strcat(ScreenString, line);
		strcat(ScreenString, ";2H\x1b[0K");
		if (FirstEvent == log){
			numline++;
			break;
		}
		log--;
		if (log < 0)
			log = TOTAL_EVENTS - 1;
		ePtr = EventLog;
		ePtr += log;
    	fseek(flash_file, (int_32)ePtr, IO_SEEK_SET);
    	read(flash_file, &WLog.TimeStamp, 8);
		if (WLog.TimeStamp != 0xffffffff){
			sprintf(&ScreenString[strlen(ScreenString)], "%3d", (EventPage * EVENTS_PER_PAGE) + a + 1);
			strcat(ScreenString, "\x1b[");
			strcat(ScreenString, line);
			strcat(ScreenString, ";8H");
			strcat(ScreenString, EventTypes[WLog.EventType]);
			strcat(ScreenString, "\x1b[");
			strcat(ScreenString, line);
			strcat(ScreenString, ";39H");
			SecondsToTime(WLog.TimeStamp, &EventTime);
			FormatDate(ScreenString, EventTime);
			strcat(ScreenString, "   ");
			FormatTime(ScreenString, EventTime);
			strcat(ScreenString, "   ");
			strcat(ScreenString, EventSource[WLog.Source]);
		}
    	numline++;
   		line[1] += 1;
   		if (line[1] > '9'){
   			line[0]++;
   			line[1] = '0';
   		}
		write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
		ScreenString[0] = 0;
	}
	while (numline < 22){
		strcat(ScreenString, "\n\r\x1b[0K");
		numline++;
	}
	strcat(ScreenString, "\x1b[22;38H");
	sprintf(&ScreenString[strlen(ScreenString)], "%d", EventPage + 1);
	write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
}

static void UpdateMemoScreen(){
	int_8 a, chars, log;
	char line[3], numline;
	struct memolog *mPtr;

	numline = 6;
	line[0] = '0';
	line[1] = '6';
	line[2] = 0;
	log = LastMemo - (EventPage * MEMO_PER_PAGE);
	if (log < 0)
		log += TOTAL_MEMO;
	for (a = 0; a < MEMO_PER_PAGE; a++){
		strcpy(ScreenString, "\x1b[");
		strcat(ScreenString, line);
		strcat(ScreenString, ";2H\x1b[0K");
		if (FirstMemo == log){
			numline++;
			break;
		}
		log--;
		if (log < 0)
			log = TOTAL_MEMO - 1;
		mPtr = MemoPad;
		mPtr += log;
    	fseek(flash_file, (int_32)mPtr, IO_SEEK_SET);
    	read(flash_file, &WorkingMemo.TimeStamp, 4);
		if (WorkingMemo.TimeStamp != 0xffffffff){
			sprintf(&ScreenString[strlen(ScreenString)], "%2d", (EventPage * MEMO_PER_PAGE) + a + 1);
			strcat(ScreenString, "\x1b[");
			strcat(ScreenString, line);
			strcat(ScreenString, ";7H");
			SecondsToTime(WorkingMemo.TimeStamp, &EventTime);
			FormatDate(ScreenString, EventTime);
			strcat(ScreenString, "   ");
			FormatTime(ScreenString, EventTime);
			strcat(ScreenString, "   ");
	    	read(flash_file, &WorkingMemo.Memo, 124);
	    	if (strlen(WorkingMemo.Memo) <= 50){
	    		strcat(ScreenString, WorkingMemo.Memo);
	    		chars = strlen(WorkingMemo.Memo);
	    	}
	    	else{
	    		strncat(ScreenString, WorkingMemo.Memo, 50);
	    		chars = 50;
	    	}
	    	numline++;
    		line[1] += 1;
    		if (line[1] > '9'){
    			line[0]++;
    			line[1] = '0';
    		}
    		strcat(ScreenString, "\x1b[");
    		strcat(ScreenString, line);
    		strcat(ScreenString, ";31H\x1b[0K");
    		if (chars < strlen(WorkingMemo.Memo)){
    			if (strlen(WorkingMemo.Memo) <= 100){
		    		strcat(ScreenString, &WorkingMemo.Memo[50]);
		    		chars = strlen(WorkingMemo.Memo);
	    		}
	    		else{
		    		strncat(ScreenString, &WorkingMemo.Memo[50], 50);
		    		chars = 100;
	    		}
    		}
    		numline++;
    		line[1] += 1;
    		if (line[1] > '9'){
    			line[0]++;
    			line[1] = '0';
    		}
    		strcat(ScreenString, "\x1b[");
    		strcat(ScreenString, line);
    		strcat(ScreenString, ";31H\x1b[0K");
    		if (chars < strlen(WorkingMemo.Memo))
	    		strcat(ScreenString, &WorkingMemo.Memo[100]);
		}
    	numline++;
   		line[1] += 1;
   		if (line[1] > '9'){
   			line[0]++;
   			line[1] = '0';
   		}
		write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
		ScreenString[0] = 0;
	}
	while (numline < 21){
		strcat(ScreenString, "\n\r\x1b[0K");
		numline++;
	}
	strcat(ScreenString, "\x1b[21;38H");
	sprintf(&ScreenString[strlen(ScreenString)], "%d", EventPage + 1);
	write(serial_fd, (char*)&ScreenString, strlen(ScreenString));
}
#endif

static void AddCircuitID(){
	strcpy(ScreenString, "\x1b[1;13H");
	strcat(ScreenString, CircuitID);
	strcat(ScreenString, "\x1b[0K");
	write(serial_fd, ScreenString, strlen(ScreenString));
}

static void WriteTime(){
	if (IncomingByteNumber == 0){				//only update if nothing is being entered
		RTC_GetTime(&RTC_TTime);
		strcpy(ScreenString, "\x1b[1;62H");
		FormatDate(ScreenString, RTC_TTime);
		strcat(ScreenString, " ");
		FormatTime(ScreenString, RTC_TTime);
		strcat(ScreenString, "\x1b[H");
		write(serial_fd, ScreenString, strlen(ScreenString));
	}
}

#ifdef FULL_CRAFT
static void SaveFiberCust(){
	_int_disable();
	if (*CustTXDisableState != TmpCustTXDis){
		*CustTXDisableState = TmpCustTXDis;
		_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
		ChangeCustDisable = true;
	}
	if (*CustTXRateState != TmpCustTXRate){
		*CustTXRateState = TmpCustTXRate;
        _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
        ChangeCustTXRate = true;
	}
	if (*CustTXPowerState != TmpCustTXPwr){
		*CustTXPowerState = TmpCustTXPwr;
    	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    	ChangeCustPower = true;
	}
	if (*CustRXRateState != TmpCustRXRate){
		*CustRXRateState = TmpCustRXRate;
    	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    	ChangeCustRXRate = true;
	}
	_int_enable();
	CustChanged = false;
}

static void SaveFiberNet(){
	_int_disable();
	if (*NetTXDisableState != TmpNetTXDis){
		*NetTXDisableState = TmpNetTXDis;
		_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
		ChangeNetDisable = true;
	}
	if (*NetTXRateState != TmpNetTXRate){
		*NetTXRateState = TmpNetTXRate;
        _io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
        ChangeNetTXRate = true;
	}
	if (*NetTXPowerState != TmpNetTXPwr){
		*NetTXPowerState = TmpNetTXPwr;
    	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    	ChangeNetPower = true;
	}
	if (*NetRXRateState != TmpNetRXRate){
		*NetRXRateState = TmpNetRXRate;
    	_io_ioctl(flash_file, FLEXNVM_IOCTL_WAIT_EERDY, NULL);
    	ChangeNetRXRate = true;
	}
	_int_enable();
	NetChanged = false;
}
#endif

void ConvertMAC(){
	uint_8 a,b;

	strcpy(ScreenString, "\x1b[20;38H");
	b = 8;
	for (a = 0; a < 12; a++){
		if (TempASCII[a] > 9)
			ScreenString[b++] = TempASCII[a++] + 0x37;
		else
			ScreenString[b++] = TempASCII[a++] + 0x30;
		if (TempASCII[a] > 9)
			ScreenString[b++] = TempASCII[a] + 0x37;
		else
			ScreenString[b++] = TempASCII[a] + 0x30;
		ScreenString[b++] = ' ';
	}
	ScreenString[b] = 0;
	strcat(ScreenString, "\x1b[20;51H");
}
#endif
