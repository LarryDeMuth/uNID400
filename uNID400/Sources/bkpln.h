/*
 * bkpln.h
 *
 *  Created on: Dec 27, 2016
 *      Author: Larry
 */

#ifndef SOURCES_BKPLN_H_
#define SOURCES_BKPLN_H_

typedef enum {
	SLAVE_IDLE = 0,
	SLAVE_RX,
	SLAVE_TX
}slavestate;

typedef enum{
	SLOT_INFO_ADDRESS = 1,			//(R) the product string, SW rev, active sfps. 				Read when slot is first active
	SFP_SUM_STATUS_ADDRESS,			//(R) SFP [1 - 4] (present, LOS, Fault, major/minor, speed)	Read every few seconds
	SFP_ID_ADDRESS,					//(R) stuff required for sfp id screen						Read when about to display ID page
	SFP_STATUS_ADDRESS,				//(R) stuff required for status screen						Read when about to display status page
	SFP_PROV_ADDRESS,				//(R/W) provision options									Read when about to display provision page / Write when done
	EVENT_COUNT_ADDRESS,			//(R) number of events										Read every few seconds, followed by reading events if any
	EVENTS_ADDRESS,					//(R) events 2 bytes per event								Read IF num events is > 0. MUST read num events from above
}addresscode;
#endif /* SOURCES_BKPLN_H_ */
