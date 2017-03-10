/*
 * Common.h
 *
 *  Created on: Mar 19, 2016
 *      Author: Larry
 */

#ifndef COMMON_H_
#define COMMON_H_

const char EventTypes[][25] =
	{"                        ",
	 "Power Lost              ",
	 "Power Up / Reset        ",
	 "Event Log Reset         ",
	 "All Memos Deleted       ",
	 "Date Changed            ",
	 "Time Changed            ",
	 "Circuit ID Changed      ",
	 "Password Changed        ",
	 "SFP Installed           ",
	 "SFP Not Installed       ",
	 "SFP Signal LOS          ",
	 "SFP Signal OK           ",
	 "SFP Comm Loss           ",
	 "SFP Comm OK             ",
	 "SFP Mismatch            ",
	 "SFP Mismatch Clear      ",
	 "SFP Speed Mismatch      ",
	 "SFP Speed OK            ",
	 "Enhanced Read Fail      ",
	 "Enhanced Read OK        ",
	 "Temp High Alarm         ",
	 "Temp Low Alarm          ",
	 "Temp High Warning       ",
	 "Temp Low Warning        ",
	 "Temp OK                 ",
	 "Voltage High Alarm      ",
	 "Voltage Low Alarm       ",
	 "Voltage High Warning    ",
	 "Voltage Low Warning     ",
	 "Voltage OK              ",
	 "TX Bias High Alarm      ",
	 "TX Bias Low Alarm       ",
	 "TX Bias High Warning    ",
	 "TX Bias Low Warning     ",
	 "TX Bias OK              ",
	 "TX Power High Alarm     ",
	 "TX Power Low Alarm      ",
	 "TX Power High Warning   ",
	 "TX Power Low Warning    ",
	 "TX Power OK             ",
	 "RX Power High Alarm     ",
	 "RX Power Low Alarm      ",
	 "RX Power High Warning   ",
	 "RX Power Low Warning    ",
	 "RX Power OK             ",
	 "Laser Temp High Alarm   ",
	 "Laser Temp Low Alarm    ",
	 "Laser Temp High Warning ",
	 "Laser Temp Low Warning  ",
	 "Laser Temp OK           ",
	 "TEC Current High Alarm  ",
	 "TEC Current Low Alarm   ",
	 "TEC Current High Warning",
	 "TEC Current Low Warning ",
	 "TEC Current OK          ",
	 "TX Fault                ",
	 "TX Fault Cleared        ",
	 "Firmware Updated        "
	};

const char EventSource[][19] =
	{"Net Primary SFP   ",
	 "Net Secondary SFP ",
	 "Cust Secondary SFP",
	 "Cust Primary SFP  ",
	 "Net & Cust SFP    ",
	 "VLX Module        "
	};

const char Compliance[][5] =
	{"",
	 "9.3",
	 "9.5",
	 "10.2",
	 "10.4",
	 "11.0",
	 "11.3",
	 "11.4",
	 "12.0"
	};

const char TransceiverType0[][15] =
	{"1X Copper Pass",
	 "1X Copper Act",
	 "1X LX",
	 "1X SX",
	 "10G Base-SR",
	 "10G Base-LR",
	 "10G Base-LRM",
	 "10G Base-ER"
	};

const char TransceiverType1[][10] =
	{"OC 48 ",
	 "OC 48 ",
	 "OC 48 ",
	 "",
	 "",
	 "OC 192 ",
	 "ESCON SMF",
	 "ESCON MMF"
	};

const char TransceiverType2[][7] =
	{"OC 3 ",
	 "OC 3 ",
	 "OC 3 ",
	 "",
	 "OC 12 ",
	 "OC 12 ",
	 "OC 12 ",
	 ""
	};

const char SONETShortReach[][11] =
	{"SONET SR",
	 "",
	 "SONET SR-1",
	 ""
	};

const char SONETInterReach[][11] =
	{"",
	 "SONET IR-2",
	 "SONET IR-1",
	 ""
	};

const char SONETLongReach[][11] =
	{"",
	 "SONET LR-2",
	 "SONET LR-1",
	 "SONET LR-3"
	};

const char TransceiverType3[][16] =
	{"1000BASE-SX",
	 "1000BASE-LX",
	 "1000BASE-CX",
	 "1000BASE-T",
	 "100BASE-LX/LX10",
	 "100BASE-FX",
	 "BASE-BX10",
	 "BASE-PX"};
/*			//dont see any reason to add these
const char TransceiverType4[][3] =
	{"EL",
	 "LC",
	 "SA",
	 "M",
	 "L",
	 "I",
	 "S",
	 "V"};

const char TransceiverType5[][8] =
	{"",
	 "",
	 "Passive",
	 "Active",
	 "LL",
	 "SL",
	 "SN",
	 "EL"};

const char TransceiverType6[][3] =
	{"SM",
	 "",
	 "M5",
	 "M6",
	 "TV",
	 "MI",
	 "TP",
	 "TW"};

const char TransceiverType7[][10] =
	{"100 MB/s",
	 "",
	 "200 MB/s",
	 "3200 MB/s",
	 "400 MB/s",
	 "1600 MB/s",
	 "800 MB/s",
	 "1200 MB/s"};
*/
const char ConnectorType[][17] =
	{"Unknown         ",
	 "FiberJack       ",
	 "LC              ",
	 "MT-RJ           ",
	 "MU              ",
	 "SG              ",
	 "Optical Pigtail ",
	 "MPO Parallel Opt",
	 "HSSDC II        ",
	 "Copper Pigtail  ",
	 "RJ45            ",
	 "Vendor Specific ",
	 "                "};

const char EncodingType[][11] =
	{"Unknown   ",
	 "8B/10B    ",
	 "4B/5B     ",
	 "NRZ       ",
	 "Manchester",
	 "SONET     ",
	 "64B/66B   ",
	 "          "};


#endif /* COMMON_H_ */
