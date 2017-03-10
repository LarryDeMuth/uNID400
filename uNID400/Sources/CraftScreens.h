/*
 * CraftScreens.h
 *
 *  Created on: Nov 25, 2014
 *      Author: Larry
 */

#ifndef CRAFTSCREENS_H_
#define CRAFTSCREENS_H_

#include "config.h"
#ifdef HAS_CRAFT
#include "common.h"

#define ESC		0x1b
#define BKSP	0x08
#define ENTER	0x0d

const char TerminalRequest[] = 
	"\x1b[5n";

const char BackDoorPassword[] = "D'?z";

const char CircuitIDField[] =
		"\x1b[0m" "\x1b[2J" "\x1b[H"
		"Circuit ID:\n\r\n\r";

const char Header[] =
		"\x1b[17C" "*****         NA Communications         *****\n\r"
		"\x1b[17C" "*****       Universal SFP Mounting      *****\n\r"
		"\x1b[17C" "*****        Model VLX-700 List C       *****\n\r\n\r";


const char Password_screen[] = 
	"\x1b[37C""LOGIN"
	"\x1b[24;19H""Enter Password: ""\x1b[7m""                ""\x1b[0m\x1b[24;35H";

const char Incorrect_Password[] = 
	"\x1b[22;27H""\x1b[0m""\x1b[1m""Incorrect Password!""\x1b[0m"
	"\x1b[24;19H""Enter Password: ""\x1b[7m""                ""\x1b[0m\x1b[24;35H";

const char maintenance_screen[] =
	"\x1b[24C"      "MANUFACTURING TEST SCREEN\n\r"
	"\n\r"
	"\x1b[20;1H" "Enter the MAC Address for this unit: "//00 50 C2 47 9X XX";
	"\x1b[22;0H" "PRESS \"I\" to Reset IP Settings (takes affect after reset)"
	"\x1b[23;0H" "PRESS \"R\" to Reset IP and Default Settings (takes affect after reset)"
	"\x1b[24;1H" "Press ESC to store MAC, ENTER to NOT store, and return";

const char MainScreen[] =
		"\x1b[35C"					"MAIN MENU\n\r\n\r"
#ifdef FULL_CRAFT
		"\x1b[27C" "\x1b[1m" "1." "\x1b[0m" "SFP Status\n\r"
		"\x1b[27C" "\x1b[1m" "2." "\x1b[0m" "SFP Identification\n\r"
		"\x1b[27C" "\x1b[1m" "3." "\x1b[0m" "SFP Provisioning\n\r"
		"\x1b[27C" "\x1b[1m" "4." "\x1b[0m" "VLX-700 Configuration\n\r"
		"\x1b[27C" "\x1b[1m" "5." "\x1b[0m" "SFP Event Log\n\r"
		"\x1b[27C" "\x1b[1m" "6." "\x1b[0m" "Memo Pad\n\r"
		"\x1b[27C" "\x1b[1m" "7." "\x1b[0m" "Contact Information\n\r"
#else
		"\x1b[27C" "\x1b[1m" "1." "\x1b[0m" "VLX-700 Configuration\n\r"
		"\x1b[27C" "\x1b[1m" "2." "\x1b[0m" "Contact Information\n\r"
#endif
		"\x1b[24;27H" "Enter Selection:";

#ifdef FULL_CRAFT
const char 	SFPStatusScreenHead[] =
		"\x1b[37C"                  "STATUS\n\r"
		"\n\r"
		"\x1b[25C"      "Network (1)""\x1b[16C"          "Customer (2)\n\r";

const char SFPStatusFiber[] =
		" SFP Status\n\r"
		" SFP RX Signal\n\r"
		" SFP TX Signal\n\r"
		" Diagnostics Supported\n\r"
		"\n\r"
		"\x1b[25C"      "Value     Status""\x1b[11C"       "Value     Status\n\r"
		" SFP Module Temperature\n\r"
		" SFP Supply Voltage\n\r"
		" SFP TX Bias Current\n\r"
		" SFP TX Output Power\n\r"
		" SFP RX Input Power\n\r"
		" SFP Laser Temperature\n\r"
		" SFP TEC Current\n\r"

		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char ClearNetFault[] =
		"\x1b[22;11H" "Press \"N\" to attempt to clear Network SFP TX Signal Fault";

const char NoNetFault[] =
		"\x1b[22;11H\x1b[0K";

const char ClearCustFault[] =
		"\x1b[23;11H" "Press \"C\" to attempt to clear Customer SFP TX Signal Fault";

const char NoCustFault[] =
		"\x1b[23;11H\x1b[0K";

const char ClearROL[] = "\x1b[K";

const char NoModulesInstalled[] =
		"\x1b[J"
		"\x1b[12;27H" "No SFP modules installed!"
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char IncompModulesInstalled[] =
		"\x1b[J"
		"\x1b[12;23H" "Incompatible SFP modules installed!"
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char NoNetModuleInstalled[] =
		"\x1b[J"
		"\x1b[12;23H" "No Network SFP module installed!"
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char NoCustModuleInstalled[] =
		"\x1b[J"
		"\x1b[12;23H" "No Customer SFP module installed!"
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char UnknownModuleInstalled[] =
		"\x1b[J"
		"\x1b[12;23H" "Unknown SFP modules installed!"
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char 	SFPIDScreen[] =
		"\x1b[33C"                  "IDENTIFICATION\n\r"
		"\n\r"
		"\x1b[14C"      "SFP-1 Network""\x1b[20C"          "SFP-2 Customer\n\r\n\r"

		"  SFP Status:\n\r"
		"  Compliance:\n\r"
		" Vendor Name:\n\r"
		" Transceiver:\n\r"
		" Part Number:\n\r"
		"    Revision:\n\r"
		"   Connector:\n\r"
		"    Encoding:\n\r"
		"    Bit Rate:\n\r"
		" Link Length\n\r"
		"        (9u):\n\r"
		"   (50u) OM2:\n\r"
		"   (50u) OM3:\n\r"
		" (62.5u) OM1:\n\r"
		"Copper Cable:\n\r"

		"\x1b[23;24H" "Press \"U\" to update identification"
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";


const char NoSFPIn[] = "Not Installed";
const char HaveSFP[] = "Installed    ";
const char SignalLOS[] = "LOS ";
const char NoLOS[] =     "OK  ";
const char Yes[] =       "YES ";
const char No[] =        "NO  ";
const char Fail[] =      "FAIL";
const char N_A[] =       "N/A ";
const char StatusNone[] =               "            ";
const char StatusOK[] =                 "OK          ";
const char StatusHighAlarm[] = "\x1b[1m""High Alarm  ""\x1b[0m";
const char StatusHighWarn[] =  "\x1b[1m""High Warning""\x1b[0m";
const char StatusLowAlarm[] =  "\x1b[1m""Low Alarm   ""\x1b[0m";
const char StatusLowWarn[] =   "\x1b[1m""Low Warning ""\x1b[0m";


const char 	SFPProvisionScreenHead[] =
		"\x1b[32C"                  "SFP PROVISIONING\n\r"
		"\n\r";

const char SFPProvisionFiber[] =
		"\x1b[33C"      "SFP-1 Network\n\r"
		"  SFP Status:\n\r"
		"\x1b[33C"		"Current\x1b[15C"        "New\n\r"
		"\x1b[1m"" 1.""\x1b[0m""TX Enable/Disable\n\r"
		"\x1b[1m"" 2.""\x1b[0m""TX Signal Rate\n\r"
		"\x1b[1m"" 3.""\x1b[0m""TX Power Level\n\r"
		"\x1b[1m"" 4.""\x1b[0m""RX Signal Rate\n\r"
		"\n\r"
		"\x1b[33C"      "SFP-2 Customer\n\r"
		"  SFP Status:\n\r"
		"\x1b[33C"		"Current\x1b[15C"        "New\n\r"
		"\x1b[1m"" 5.""\x1b[0m""TX Enable/Disable\n\r"
		"\x1b[1m"" 6.""\x1b[0m""TX Signal Rate\n\r"
		"\x1b[1m"" 7.""\x1b[0m""TX Power Level\n\r"
		"\x1b[1m"" 8.""\x1b[0m""RX Signal Rate\n\r"

		"\x1b[22;14H" "Press\"N\" to update Network SFP, \"C\" to update Customer SFP"
		"\x1b[23;24H" "Press number to change selection."
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char Enabled[] =  "Enabled ";
const char Disabled[] = "Disabled";
const char LowRate[] =  "Low Rate ";
const char HighRate[] = "High Rate";
const char PowerLow[] = "Low Power ";
const char PowerHigh[] = "High Power";

const char NotImplemented[] = "\x1b[0K\x1b[7CNot Implemented";

const char NCNotSaved[] = "\x1b[22;1H" "\x1b[1;5m" "Network and Customer settings NOT saved! Press ENTER to save, ESC to not save.""\x1b[0m";
const char NNotSaved[] = "\x1b[22;8H" "\x1b[1;5m" "Network settings NOT saved! Press ENTER to save, ESC to not save.""\x1b[0m";
const char CNotSaved[] = "\x1b[22;8H" "\x1b[1;5m" "Customer settings NOT saved! Press ENTER to save, ESC to not save.""\x1b[0m";

#endif

const char ConfigurationScreen[] =
		"\x1b[30C"             "VLX-700 CONFIGURATION\n\r"
		"\x1b[21C"      "Current""\x1b[24C"            "New\n\r\n\r"
		"\x1b[1m"" 1.""\x1b[0m""IP Address\n\r"
		"\x1b[1m"" 2.""\x1b[0m""Subnet Mask\n\r"
		"\x1b[1m"" 3.""\x1b[0m""Gateway IP\n\r"
		"\x1b[1m"" 4.""\x1b[0m""Current Date:\x1b[20C" "(MM/DD/YYYY)\n\r"
		"\x1b[1m"" 5.""\x1b[0m""Current Time:\x1b[20C" "(HH:MM:SS)\n\r"
		"\x1b[1m"" 6.""\x1b[0m""Circuit ID:\n\r"
		"\x1b[1m"" 7.""\x1b[0m""Change Password\n\r"

		"\x1b[19;12H" "Valid Ranges: MM - 01 to 12""\x1b[14C""HH - 00 to 23\n\r"
		"\x1b[20;26H" "DD - 01 to 31""\x1b[14C""MM - 00 to 59\n\r"
		"\x1b[21;26H" "YYYY - 2000 to 2199""\x1b[8C""SS - 00 to 59\n\r"
		"\x1b[23;24H" "Press number to change selection."
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char CursorAtIP[] =
"\x1b[23;19H" "Press ENTER to STORE new IP address""\x1b[0K"
"\x1b[24;19H" "Press ESC to CANCEL changes""\x1b[0K"
"\x1b[6;53H";

const char CursorAtSubnet[] =
"\x1b[23;19H" "Press ENTER to STORE new Subnet Mask""\x1b[0K"
"\x1b[24;19H" "Press ESC to CANCEL changes""\x1b[0K"
"\x1b[7;53H";

const char CursorAtGateway[] =
"\x1b[23;19H" "Press ENTER to STORE new Gateway IP address""\x1b[0K"
"\x1b[24;19H" "Press ESC to CANCEL changes""\x1b[0K"
"\x1b[8;53H";

const char InvalidSubnet[] =
"\x1b[H\x1b[10B\x1b[33C"
"\x1b[22;19H" "Invalid Subnet Mask Entered!!"
"\x1b[23;19H" "Press ENTER to STORE new Subnet Mask""\x1b[0K"
"\x1b[24;19H" "Press ESC to CANCEL changes""\x1b[0K"
"\x1b[7;53H";

const char CursorAtDate[] =
		"\x1b[23;24H""Enter Current Date (MM/DD/YYYY) then press ENTER."
		"\x1b[24;24H" "Press ESC to Cancel changes\x1b[0K"
		"\x1b[9;53H";

const char CursorAtTime[] =
		"\x1b[23;24H""Enter Current Time (HH:MM:SS) then press ENTER.\x1b[0K"
		"\x1b[24;24H" "Press ESC to Cancel changes\x1b[0K"
		"\x1b[10;53H";

const char CursorAtCircuitID[] =
		"\x1b[23;24H""Enter Circuit ID then press ENTER.\x1b[0K"
		"\x1b[24;24H" "Press ESC to Cancel changes\x1b[0K"
		"\x1b[11;53H";

const char CancelConfig[] =
	    "\x1b[6;53H\x1b[0K"
	    "\x1b[7;53H\x1b[0K"
	    "\x1b[8;53H\x1b[0K"
	    "\x1b[9;53H\x1b[0K"
	    "\x1b[10;53H\x1b[0K"
	    "\x1b[11;53H\x1b[0K"
		"\x1b[22;19H\x1b[0K"
		"\x1b[23;24H" "Press number to change selection.\x1b[0K"
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char CursorAtPassword[] =  //Header added first
	"\x1b[13;18H""Old Password:\n\r"
	"\x1b[17C"   "New Password:\n\r"
	"\x1b[10C""Verify New Password:\n\r"
	"\x1b[23;24H" "Enter old password and press ENTER\x1b[0K"
	"\x1b[24;24H" "Press ESC to return to Cancel\x1b[0K"
    "\x1b[13;32H";

const char CursorAtCurPassword[] =
	"\x1b[23;24H" "Enter old password and press ENTER "
    "\x1b[13;32H";

const char CursorAtPassword1[] =
	"\x1b[23;24H" "Enter new password and press ENTER "
    "\x1b[14;32H";

const char CursorAtPassword2[] =
	"\x1b[23;24H" "Verify new password and press ENTER"
    "\x1b[15;32H";

const char InvalidPassword[] =
	"\x1b[23;24H\x1b[0K""\x1b[1m""      Invalid Password!""\x1b[0m"
	"\x1b[13;32H\x1b[0K""\x1b[13;32H";

const char PasswordNoMatch[] =
	"\x1b[23;24H\x1b[0K""\x1b[1m""      Passwords dont match!""\x1b[0m"
	"\x1b[14;32H\x1b[0K"
	"\x1b[0m""\x1b[15;32H\x1b[0K""\x1b[14;32H";

const char PasswordChanged[] =
    "\x1b[13;1H\x1b[0K"
    "\x1b[14;1H\x1b[0K"
    "\x1b[15;1H\x1b[0K"
	"\x1b[23;24H" "Press number to change selection.\x1b[0K"
	"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[0K\x1b[H";

#ifdef FULL_CRAFT
const char EventLogScreen[] =
		"\x1b[35C"                  "EVENT LOG\n\r"
		" Num   Description of Events          Date         Time       Source\n\r"
		"--------------------------------------------------------------------------------"
		"\x1b[22;33H"                    "Page   of 7"
		"\x1b[23;7H" "Press \"N\" for Next Page, \"P\" for Previous Page, or \"R\" to Reset Log."
		"\x1b[24;24H" "Press ESC to return to Main Menu";


const char MemoPadScreen[] =
		"\x1b[35C"                  "MEMO PAD\n\r"
		" Num  Date         Time       Memo\n\r"
		"--------------------------------------------------------------------------------"
		"\x1b[21;33H"                    "Page   of 7"
		"\x1b[22;15H" "Press \"N\" for Next Page or \"P\" for Previous page."
		"\x1b[23;15H" "Press \"A\" to Add a memo, or \"D\" to Delete all memo's."
		"\x1b[24;24H" "Press ESC to return to Main Menu";

const char ConfirmDelete[] =
		"\x1b[22;15H\x1b[0K"
		"\x1b[23;15H" "CONFIRM DELETE! Press ENTER to Delete all memo's.\x1b[0K"
		"\x1b[24;24H" "Press ESC to Cancel\x1b[0K\x1b[H";


const char CancelDelete[] =
		"\x1b[22;15H" "Press \"N\" for Next Page or \"P\" for Previous page."
		"\x1b[23;15H" "Press \"A\" to Add a memo, or \"D\" to Delete all memo's."
		"\x1b[24;24H" "Press ESC to return to Main Menu\x1b[H";

const char AddMemoScreen[] =
		"\x1b[0m" "\x1b[2J" "\x1b[H"
		"Circuit ID:\n\r\n\r"

		"\x1b[35C"                  "ADD MEMO"
		"\x1b[10;1H\x1b[5m_\x1b[0m"
		"\x1b[17;10H" "123 characters max. Enter is NOT a valid character in the memo."
 		"\x1b[23;24H" "Press ENTER to Save memo and return to Memo Pad."
		"\x1b[24;24H" "Press ESC to return to Memo Pad";
#endif

const char ContactSreen[]=
		"\x1b[30C"             "CONTACT INFORMATION\n\r"
		"\n\r"
		"\x1b[30C"            "NA Communications\n\r"
		"\x1b[30C"            "3820 Ohio Ave. Unit 12\n\r"
		"\x1b[30C"            "St. Charles, IL 60174\n\r"
		"\n\r"
		"\x1b[30C"            "Technical Support: 1-888-402-4622\n\r"
		"\n\r"
		"\x1b[30C"             "Web:               www.gotonac.com\n\r"
		"\n\r"
		"\x1b[30C"             "Firmware revision:  "SWREV"\n\r"
		"\x1b[24;24H" "Press ESC to return to the Main Menu";


const char BackSpace[] = 
	"\x08\x20\x08";

const char DoubleBackSpace[] =
	"\x08\x08\x20\x08";

#endif
#endif /* CRAFTSCREENS_H_ */
