/*
 * config.h
 *
 *  Created on: Nov 25, 2014
 *      Author: Larry
 */

#ifndef CONFIG_H_
#define CONFIG_H_

//#define SHOW_HEARTBEAT			//this is just for testing. undefine this for production


#define SWREV	"1.0.0"								//10 chars MAX
const char ProductString[] = "NAC uNID400";			//32 chars MAX

//#define HAS_EXT_RTC		1
//#define HAS_INTERNAL_ELOG	1
//#define HAS_MEMO_PAD		1
#define USE_BKPLN_TASK		1

#ifdef HAS_INTERNAL_ELOG
#ifndef HAS_EXT_RTC
#error "Event log requires HAS_EXT_RTC to be defined in config.h"
#endif
#endif

//I2C PIN DEFINES
#define SFP_SCL						0x1000
#define SFP_SDA						0x0800

//SFP PIN DEFINES
#define RATE_SELECT_PIN				0x1
#define TX_DISABLE_PIN				0x2
#define FAULT_PIN					0x4

#define CUST_S_PIN					0x1
#define CUST_P_PIN					0x2
#define NET_S_PIN					0x4
#define NET_P_PIN					0x8

//UART DEFINES
#define MAX_BUFFER 					2048
#define RCVINT_XMT_RCV_ENABLED 		0x2c
#define RCV_XMT_INT_XMT_RCV_ENABLED	0x2C
#define TC_INT_ENABLED				0x40
#define FLUSH_XMT_RCV_BUFFER 		0xc0
#define FLUSH_RCV_BUFFER			0x40
#define TRANSMIT_COMPLETE 			0x40
#define PARITY_ERROR				0x40
#define RECEIVE_BUFFER_FULL 		0x20
#define RECEIVE_BUFFER_OVERFLOW 	0x08

typedef enum{
	SFP_NONE = 0,
	SFP_NORMAL,
	SFP_T1,
	SFP_T3,
	SFP_USB
}sfptype;

//event and memo defines
#ifdef HAS_INTERNAL_ELOG
#define TOTAL_EVENTS		1024
#define MAX_EVENTS			100
#define EVENTS_PER_PAGE		15
#else
#define MAX_EVENTS			25
#endif

#define MAX_MEMO		32
#define TOTAL_MEMO		64
#define MEMO_PER_PAGE	5

//event enumerates MUST MATCH craftscreens.h!!!
typedef enum{
	SOURCE_NET_P_SFP = 0,
	SOURCE_NET_S_SFP,
	SOURCE_CUST_S_SFP,
	SOURCE_CUST_P_SFP,
	SOURCE_NET_CUST_SFP,
	SOURCE_VLX_MODULE
}eventsource;

typedef enum{
	EVENT_NONE = 0,
	EVENT_POWERDOWN,
	EVENT_POWERUP,
	EVENT_LOG_RESET,
	EVENT_MEMO_RESET,
	EVENT_DATE_CHANGED,
	EVENT_TIME_CHANGED,
	EVENT_CIRCUIT_ID_CHANGED,
	EVENT_PASSWORD_CHANGED,
	EVENT_SFP_INSTALLED,
	EVENT_SFP_REMOVED,
	EVENT_SIGNAL_LOS,
	EVENT_SIGNAL_OK,
	EVENT_SFP_COMM_LOST,
	EVENT_SFP_COMM_OK,
	EVENT_SFP_MISMATCH,
	EVENT_SFP_MISMATCH_CLEAR,
	EVENT_SFP_SPEED_BAD,
	EVENT_SFP_SPEED_GOOD,
	EVENT_ENHANCED_FAIL,
	EVENT_ENHANCED_OK,
	EVENT_TEMP_HI_ALARM,
	EVENT_TEMP_LO_ALARM,
	EVENT_TEMP_HI_WARN,
	EVENT_TEMP_LO_WARN,
	EVENT_TEMP_OK,
	EVENT_VOLT_HI_ALARM,
	EVENT_VOLT_LO_ALARM,
	EVENT_VOLT_HI_WARN,
	EVENT_VOLT_LO_WARN,
	EVENT_VOLT_OK,
	EVENT_BIAS_HI_ALARM,
	EVENT_BIAS_LO_ALARM,
	EVENT_BIAS_HI_WARN,
	EVENT_BIAS_LO_WARN,
	EVENT_BIAS_OK,
	EVENT_TX_PWR_HI_ALARM,
	EVENT_TX_PWR_LO_ALARM,
	EVENT_TX_PWR_HI_WARN,
	EVENT_TX_PWR_LO_WARN,
	EVENT_TX_PWR_OK,
	EVENT_RX_PWR_HI_ALARM,
	EVENT_RX_PWR_LO_ALARM,
	EVENT_RX_PWR_HI_WARN,
	EVENT_RX_PWR_LO_WARN,
	EVENT_RX_PWR_OK,
	EVENT_LASER_HI_ALARM,
	EVENT_LASER_LO_ALARM,
	EVENT_LASER_HI_WARN,
	EVENT_LASER_LO_WARN,
	EVENT_LASER_OK,
	EVENT_TEC_HI_ALARM,
	EVENT_TEC_LO_ALARM,
	EVENT_TEC_HI_WARN,
	EVENT_TEC_LO_WARN,
	EVENT_TEC_OK,
	EVENT_TX_FAULT,
	EVENT_TX_FAULT_CLEAR,
	EVENT_FW_UPDATE
}eventypes;

//SFP ADDRESS DEFINES
#define SFP_ADDRESS			0x50	//actually a0 but it gets shifted left one place, so 50
#define SFP_PLUS_ADDRESS	0x51	//same here, actually a2.
#define SMART_SFP_ADDRESS	0x52 //DONT KNOW THIS ONE YET!!!
#define SFP_MUX_ADDRESS		0x70	//actually e0.
#define PAGE_CHANGE_ADDRESS 0x00	//this is for changing addresses on some SFP's
#define SFP_ADDR_CHANGE_REG	0x04

//SFP REGISTERS ADDRESS A0
#define SFP_IDENTIFIER			0
#define EXT_IDENTIFIER			1
#define CONNECTOR				2
#define TRANSCEIVER				3
#define ENCODING				11
#define NOMINAL_BR				12
#define RATE_ID					13
#define LENGTH_SMF_KM			14
#define LENGTH_SMF				15
#define LENGTH_OM2				16
#define LENGTH_OM1				17
#define LENGTH_COPPER			18
#define LENGTH_OM3				19
#define VENDOR_NAME				20
#define NOT_USED_1				36
#define VENDOR_OUI				37
#define VENDOR_PN				40
#define VENDOR_REV				56
#define WAVELENGTH				60
#define NOT_USED_2				62
#define CC_BASE					63
#define SFP_OPTIONS				64
#define BR_MAX					66
#define BR_MIN					67
#define VENDOR_SN				68
#define DATE_CODE				84
#define DIAG_MON_TYPE			92
#define ENHANCED_OPTIONS		93
#define SF_8472_COMPLIANCE		94
#define CC_EXT					95
#define VENDOR_SPECIFIC			96
#define SFP_RESERVED_1			128

//A0 identifier (address 0)
#define SFP_SFP_PLUS			0x03
#define DWDM_SFP				0x0b
#define QSFP					0x0c
#define QSFP_PLUS				0x0d

//A0 rate id (address 13)
#define SFF_8079_RATE_SELECT		0x01
#define SFF_8431_RX_SEL_ONLY		0x02
#define SFF_8431_TX_SEL_ONLY		0x04
#define SFF_8431_TX_RX_SEL			0x06
#define FC_PI_5_RX_SEL_ONLY			0x08
#define FC_PI_5_TX_RX_SEL			0x0a
#define FC_PI_6_TX_RX_SEL			0x0c
#define DWDM_TX_RX_SEL				0x0e

//A0 options (address 64, 65)
#define POWER_LEVEL_HIGH			0x2000	//works with power level low
#define PAGING_IMPLEMENTED			0x1000
#define RETIMER_CDR_IMPLEMENTED		0x0800
#define COOLED_LASER_IMPLEMENTED	0x0400
#define POWER_LEVEL_LOW				0x0200 //works with power level high
#define LINEAR_RECEIVER_IMPLEMENTED	0x0100
#define RDT_IMPLEMENTED				0x0080
#define TUNEABLE_XMT_IMPLEMENTED	0x0040
#define RATE_SELECT_IMPLEMENTED		0x0020
#define TX_DISABLE_IMPLEMENTED		0x0010
#define TX_FAULT_IMPLEMENTED		0x0008
#define LOS_INVERTED_IMPLEMENTED	0x0004	//NOT STANDARD AND SHOULD BE AVOIDED
#define LOS_IMPLEMENTED				0x0002


//A0 diagnostic monitoring type flags (address 92)
#define ADDRESS_CHANGE_REQUIRED	0x04
#define RX_POWER_AVERAGE		0x08
#define EXTERNAL_CALIBRATION	0x10
#define INTERNAL_CALIBRATION	0x20
#define SFP_DDM_IMPLEMENTED 	0x40

//A0 enhanced options flag (address 93)
//C = control, M = monitoring
#define RATE_SELECT_C_IMPLEMENTED	0x01
#define APP_SELECT_C_IMPLEMENTED	0x04	//not sure what this is
#define SOFT_RATE_C_M_IMPLEMEMTED	0x08
#define RX_LOS_M_IMPLEMENTED		0x10
#define TX_FAULT_M_IMPLEMENTED		0x20
#define TX_DISABLE_C_M_IMPLEMENTED	0x40
#define FLAGS_IMPLEMENTED			0x80

//A0 SFF-8472 compliance (address 94)
typedef enum{
	COMPLIANCE_UNDEFINED = 0,
	COMPLIANCE_9_3,
	COMPLIANCE_9_5,
	COMPLIANCE_10_2,
	COMPLIANCE_10_4,
	COMPLIANCE_11_0,
	COMPLIANCE_11_3,
	COMPLIANCE_11_4,
	COMPLIANCE_12_0,
	COMPLIANCE_INVALID	//THIS SHOULD ALWAYS BE LAST!!
}compliance;

//SFP REGISTERS ADDRESS A2
#define AW_THRESHOLDS			0
#define EXT_CAL_CONSTANTS		56
#define UNALLOCATED_2			92
#define CC_DMI					95
#define DIAGNOSTICS				96
#define STATUS_CONTROL			110
#define SFP_RESERVED_2			111
#define ALARM_FLAGS				112
#define UNALLOCATED_4			114
#define WARNING_FLAGS			116
#define EXT_STATUS_CONTROL		118
#define VENDOR_SPECIFIC_A2		120
#define USER_EEPROM				128
#define VENDOR_CONTROL			248

//A2 status/control defines (register 110)
#define RX_LOS_PIN_STATE		0x02
#define TX_FAULT_PIN_STATE		0x04
#define SOFT_RS0_RATE_SELECT	0x08
#define RS0_RATE_PIN_STATE		0x10
#define RS1_RATE_PIN_STATE		0x20
#define SOFT_TX_DISABLE			0x40
#define TX_DISABLE_PIN_STATE	0x80

//A2 alarm warning flags (register alarm = 112 and 113, warning = 116 and 117)
#define TEMP_HIGH				0x8000
#define TEMP_LOW				0x4000
#define VCC_HIGH				0x2000
#define VCC_LOW					0x1000
#define TX_BIAS_HIGH			0x0800
#define TX_BIAS_LOW				0x0400
#define TX_POWER_HIGH			0x0200
#define TX_POWER_LOW			0x0100
#define RX_POWER_HIGH			0x0080
#define RX_POWER_LOW			0x0040
#define LASER_TEMP_HIGH			0x0020
#define LASER_TEMP_LOW			0x0010
#define TEC_CURRENT_HIGH		0x0008
#define TEC_CURRENT_LOW			0x0004

//a2 extended status/control (address 118, 119)
#define SOFT_RS1_RATE_SELECT	0x0800
#define POWER_LEVEL_STATUS		0x0200
#define POWER_LEVEL_SELECT		0x0100
#define TX_CDR_UNLOCKED			0x0002
#define RX_CDR_UNLOCKED			0x0001

//for screens
#define RX_RS			0x01		//from rate identifier (A0 register 13) is rate select implemented
#define TX_RS			0x02		//from rate identifier (A0 register 13)
#define TX_DISABLE		0x04		//from options (A0 register 65) is it implemented
#define TX_FAULT		0x08		//from options (A0 register 65) is it implemented
#define RX_SOFT_RS		0x10		//from enhanced options (A0 register 93) (rate select is for TX and RX?)
#define TX_SOFT_RS		0x20		// "
#define TX_SOFT_DISABLE	0x40		// "
#define TX_SOFT_FAULT	0x80		// "

#define ENABLE_TX					1
#define DISABLE_TX					2

#define LOW_RATE_RX					0x1
#define HIGH_RATE_RX				0x2

#define LOW_RATE_TX					0x10
#define HIGH_RATE_TX				0x20

#define LOW_POWER					1
#define HIGH_POWER					2

//SFP flags defines for SFPData.Flags
#define A0_READ						0x1
#define DDM_IMPLEMENTED				0x2
#define ADDR_CHANGE					0x4
#define FIRST_READ_A2				0x8
#define CHANGE_DISABLE				0x10
#define CHANGE_RX_RATE				0x20
#define CHANGE_TX_RATE          	0x40
#define CHANGE_POWER				0x80
#define SFP_EVENT_SFP_IN			0x100
#define SFP_EVENT_COMM_OK  			0x200
#define SFP_EVENT_TX_FAULT			0x400
#define SFP_EVENT_LOS				0x800
#define SFP_EVENT_A2_OK				0x1000
#define SFP_EVENT_SPEED_MISMATCH	0x2000
#define CLEAR_FAULT					0x8000

//IC2 defines
#define MAX_BUSY_WAIT		50
#define TRANSMIT_TIMEOUT	2
#define I2C_RECEIVE_TIMEOUT	4


//LED DEFINES
typedef enum{
	LED_OFF = 0,
	LED_ON,
	LED_PULSE,
	LED_BLINK
}ledstate;

#define LED_LOW_FIELD(x) (uint_32)(x & 0x1F)
#define LED_MEDIUM_FIELD(x) (uint_32)((x << 5) & 0xFFF)
#define LED_HIGH_FIELD(x) (uint_32)((x << 17) & 0x1F)

//these are the LEDS that may flash or blink
typedef enum{
	CUST_S_OP = 0,			//LOW REG (PORT A LED_Lo Field)
	CUST_S_PROB,			//	"
	CUST_S_SPEED,			//	"
	CUST_S_LOS,				//	"
	CUST_P_OP,				//	"
	CUST_P_PROB,			//MEDIUM REG (PORT B LED_Med Field)
	CUST_P_SPEED,			//	"
	CUST_P_LOS,				//	"
	NET_S_OP,				//	"
	NET_S_PROB,				//	"
	NET_S_SPEED,			//	"
	NET_S_LOS,				//	"
	NET_P_OP,				//	"
	NET_P_PROB,				//	"
	NET_P_SPEED,			//	"
	NET_P_LOS,				//	"
	PROT_LED,				//	"
	MON_LED,				//HIGH REG (PORT B LED_Hi Field)
	NID_MINOR,				//	"
	NID_MAJOR,				//	"
	PWR_LED,				//	"
	PWR_RST,				//	"
	FLASHING_LED_END		//this should ALWAYS be last
}flashingleds;

//login defines
typedef enum{
	NOT_LOGGED_IN = 0,
	MAINT_LOGIN,
	SETUP_LOGIN
}loginstate;
//MUX DEFINES
//Registers
#define GLOBAL_MUX					0x50
#define GLOBAL_INPUT_GAIN_1			0x51
#define GLOBAL_INPUT_GAIN_2			0x52
#define GLOBAL_INPUT_STATE			0x54
	#define STATE_DEFAULT				0x4C
#define GLOBAL_INPUT_LOS			0x55
#define GLOBAL_OUTPUT_PE_1			0x56
#define GLOBAL_OUTPUT_PE_2			0x57
#define GLOBAL_OUTPUT_LEVEL			0x58
	#define LEVEL_DEFAULT				0xB2
#define GLOBAL_OUTPUT_MODE			0x59
	#define MODE_DEFAULT				0x02
#define GLOBAL_INPUT_ISE_1_LONG		0x5A
#define GLOBAL_INPUT_ISE_1_SHORT	0x5B
#define GLOBAL_INPUT_ISE_2_LONG		0x5C
#define GLOBAL_INPUT_ISE_2_SHORT	0x5D

//RTC DEFINES
//registers
#define RTC_SECONDS_FRACTION		0x00
#define RTC_SECONDS					0x01
#define RTC_MINUTES					0x02
#define RTC_HOUR					0x03
#define RTC_DAY						0x04
#define RTC_DATE					0x05
#define RTC_MONTH					0x06
#define RTC_YEAR					0x07
#define RTC_CONTROL_REG				0x08
#define RTC_CALIBRATION				0x09
#define RTC_ALARM_0_SECONDS			0x0C
#define RTC_ALARM_0_MINUTES			0x0D
#define RTC_ALARM_0_HOURS			0x0E
#define RTC_ALARM_0_DAY				0x0F
#define RTC_ALARM_0_DATE			0x10
#define RTC_ALARM_0_MONTH			0x11
#define RTC_ALARM_1_SECONDS_FRAC	0x12
#define RTC_ALARM_1_SECONDS			0x13
#define RTC_ALARM_1_MINUTES			0x14
#define RTC_ALARM_1_HOURS			0x15
#define RTC_ALARM_1_DAY				0x16
#define RTC_ALARM_1_DATE			0x17
#define RTC_PD_MINUTES				0x18
#define RTC_PD_HOURS				0x19
#define RTC_PD_DATE					0x1A
#define RTC_PD_MONTH				0x1B
#define RTC_PU_MINUTES				0x1C
#define RTC_PU_HOURS				0x1D
#define RTC_PU_DATE					0x1E
#define RTC_PU_MONTH				0x1F
//SPI Instructions
#define RTC_SRWRITE					0x01
#define RTC_EEWRITE					0x02
#define RTC_EEREAD					0x03
#define RTC_EEWRDI					0x04
#define RTC_SRREAD					0x05
#define RTC_EEWREN					0x06
#define RTC_WRITE					0x12
#define RTC_READ					0x13
#define RTC_UNLOCK					0x14
#define RTC_IDWRITE					0x32
#define RTC_IDREAD					0x33
#define RTC_CLRRAM					0x54


//changing these will cause certain defaults to be set
//they are divided up so one change to one part doesn't require ALL to be defaulted
//#define MAC_TEST		0x75    //changing this will cause MAC address to be defaulted. DON'T CHANGE THIS!
//#define IP_TEST			0x50	//changing this will cause ip settings and password to be defaulted
#define SETTINGS_TEST	0x24    //changing this will cause unit settings to be defaulted
#define PM_DATA_TEST	0x84	//changing this will cause PM registers to be defaulted

//FlexRAM defines 		This is used for TFTP to save the new code while transferring
#define SRECFilePtr				((uint_8*)0x10000000)

//these start with 0x0 instead of 0x1 because they are relative to flash bank 1, not an absolute address (0x00018000 is MAX!)
//THESE ARE DEFAULTED BY CHANGING PM_DATA_TEST OR BY SOFTWARE COMMAND
#ifdef HAS_MEMO_PAD
#ifdef HAS_INTERNAL_ELOG
#define END_FLEXRAM				((uint_8*)0x00014000)
#define MemoPad							((struct memolog*) 0x00014000)	//128 bytes * 64 memo's = 8192 bytes (2 sectors)
//next 0x00016000
#define EventLog						((struct eventlog*)0x00016000)	//8 bytes * 1024 events = 8192 bytes (2 sectors)
//next 0x00018000
#else
#define END_FLEXRAM				((uint_8*)0x00016000)
#define MemoPad							((struct memolog*) 0x00016000)	//128 bytes * 64 memo's = 8192 bytes (2 sectors)
#endif
#else
#ifdef HAS_INTERNAL_ELOG
#define END_FLEXRAM				((uint_8*)0x00016000)
#define EventLog						((struct eventlog*)0x00016000)	//8 bytes * 1024 events = 8192 bytes (2 sectors)
#else
#define END_FLEXRAM				((uint_8*)0x00018000)
#endif
#endif

#define SER_SREC_MAX_SIZE		END_FLEXRAM - SRECFilePtr

//FlexNVRAM defines		These are our settings that must be saved (0x14000000 = 0x14000100)
//must start on a 4 byte boundary (x0, x4, x8, xC) unless its just bytes
//#define MACState						((uint_8*)0x14000000)			//1 byte if not set, MAC address will default
//#define IPState							((uint_8*)0x14000001)			//1 byte if not set, IP address settings will default
#define SettingsState					((uint_8*)0x14000002)			//1 byte. If not set, settings will be defaulted
#define PMStatsState					((uint_8*)0x14000003)			//1 byte. If not set, PM Stats will be defaulted

//THIS IS DEFAULTED BY CHANGING MAC_TEST		dont change it! MAC IS IN THE SAME LOCATION ON ALL VLX's
//#define MACAddress						((uint_8*)0x14000004)			//6 bytes

//THESE ARE DEFAULTED BY CHANGING IP_TEST OR BY SOFTWARE COMMAND
//#define LocalIPAddress					((uint_8*)0x1400000A)			//4 bytes
//#define GatewayIPAddress				((uint_8*)0x1400000E)			//4 bytes
//#define SubnetMask						((uint_8*)0x14000012)			//4 bytes
#define Password						((char*)0x14000016)				//17 bytes

//THESE ARE DEFAULTED BY CHANGING SETTINGS_TEST OR BY SOFTWARE COMMAND
#define CircuitID						((char*)0x14000030)				//28 bytes

//NET
//FIBER
#define NetTXDisableState				((char*)0x14000050)
#define NetTXRateState					((char*)0x14000051)
#define NetTXPowerState					((char*)0x14000052)
#define NetRXRateState					((char*)0x14000053)

//CUST
//FIBER
#define CustTXDisableState				((char*)0x14000070)
#define CustTXRateState					((char*)0x14000071)
#define CustTXPowerState				((char*)0x14000072)
#define CustRXRateState					((char*)0x14000073)

#endif /* CONFIG_H_ */
