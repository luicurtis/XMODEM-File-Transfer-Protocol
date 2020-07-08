//============================================================================
//
//% Student Name 1: Sina Ahmadian Behrouz
//% Student 1 #: 301292337
//% Student 1 userid (email): ahmadian (ahmadian@sfu.ca)
//
//% Student Name 2: Curtis Lui
//% Student 2 #: 301304667
//% Student 2 userid (email): cwlui (cwlui@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P2_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : SenderX.cpp
// Version     : September 3rd, 2019
// Description : A solution for ENSC 351 Project Part 2
// Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

//#include <iostream>
#include <stdint.h> // for uint8_t
#include <string.h> // for memset()
#include <fcntl.h>	// for O_RDONLY
#include <memory>
#include <thread>
#include <chrono>

#include "AtomicCOUT.h"
#include "myIO.h"
#include "SenderX.h"
#include "VNPE.h"
#include "SenderSS.h"

// comment out the line below to get rid of Sender logging information.
#define REPORT_INFO

using namespace std;
using namespace Sender_SS;

SenderX::SenderX(const char *fname, int d)
:PeerX(d, fname),
 bytesRd(-2), // initialize with unique value.
 firstCrcBlk(true),
 blkNum(0)  	// but first block sent will be block #1, not #0
{
}

//-----------------------------------------------------------------------------

// Send the block, less the block's last byte, to the receiver.
// Returns the block's last byte.

//uint8_t SenderX::sendMostBlk(blkT blkBuf)
uint8_t SenderX::sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	const int mostBlockSize = (this->Crcflg ? BLK_SZ_CRC : BLK_SZ_CS) - 1;
	PE_NOT(myWrite(mediumD, blkBuf, mostBlockSize), mostBlockSize);
	return *(blkBuf + mostBlockSize);
}

// Send the last byte of a block to the receiver
void
SenderX::
sendLastByte(uint8_t lastByte)
{
	PE(myTcdrain(mediumD)); // wait for previous part of block to be completely drained from the descriptor
	dumpGlitches();			// dump any received glitches

	PE_NOT(myWrite(mediumD, &lastByte, sizeof(lastByte)), sizeof(lastByte));
}

/* tries to generate a block.  Updates the
variable bytesRd with the number of bytes that were read
from the input file in order to create the block. Sets
bytesRd to 0 and does not actually generate a block if the end
of the input file had been reached when the previously generated block
was prepared or if the input file is empty (i.e. has 0 length).
*/
void SenderX::genBlk(blkT blkBuf)
//void SenderX::genBlk(uint8_t blkBuf[BLK_SZ_CRC])
{
	//read data and store it directly at the data portion of the buffer
	bytesRd = PE(myRead(transferringFileD, &blkBuf[DATA_POS], CHUNK_SZ ));
	if (bytesRd>0) {
		blkBuf[0] = SOH; // can be pre-initialized for efficiency
		//block number and its complement
		uint8_t nextBlkNum = blkNum + 1;
		blkBuf[SOH_OH] = nextBlkNum;
		blkBuf[SOH_OH + 1] = ~nextBlkNum;
		if (this->Crcflg) {
			/*add padding*/
			if(bytesRd<CHUNK_SZ)
			{
				//pad ctrl-z for the last block
				uint8_t padSize = CHUNK_SZ - bytesRd;
				memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);
			}

			/* calculate and add CRC in network byte order */
			crc16ns((uint16_t*)&blkBuf[PAST_CHUNK], &blkBuf[DATA_POS]);
		}
		else {
			//checksum
			blkBuf[PAST_CHUNK] = blkBuf[DATA_POS];
			for( int ii=DATA_POS + 1; ii < DATA_POS+bytesRd; ii++ )
				blkBuf[PAST_CHUNK] += blkBuf[ii];

			//padding
			if( bytesRd < CHUNK_SZ )  { // this line could be deleted
				//pad ctrl-z for the last block
				uint8_t padSize = CHUNK_SZ - bytesRd;
				memset(blkBuf+DATA_POS+bytesRd, CTRL_Z, padSize);
				blkBuf[PAST_CHUNK] += CTRL_Z * padSize;
			}
		}
	}
}

/* tries to prepare the first block.
*/
void SenderX::prep1stBlk()
{
	// **** this function will need to be modified ****
    // TODO

	// maybe better to use something derived from variable blkNum instead of 1 below
	genBlk(blkBufs[1]);
}

/* refit the 1st block with a checksum
*/
void
SenderX::
cs1stBlk()
{
	// **** this function will need to be filled in ****
    // TODO

	// maybe better to use something derived from variable blkNum instead of 1 below
	checksum (&blkBufs[1][PAST_CHUNK], blkBufs[1]);
}

/* while sending the now current block for the first time, prepare the next block if possible.
*/
void SenderX::sendBlkPrepNext()
{
	// **** this function will need to be modified ****
	blkNum ++; // 1st block about to be sent or previous block ACK'd
#ifdef REPORT_INFO
	// block will be "w"ritten to mediumD
	COUT << "\n[w" << (int)blkNum << "]" << flush;
#endif
	uint8_t lastByte = sendMostBlk(blkBufs[blkNum%2]);
	genBlk(blkBufs[(blkNum+1)%2]); // prepare next block
	sendLastByte(lastByte);
}

// Resends the block that had been sent previously to the xmodem receiver
void SenderX::resendBlk()
{
	// resend the block including the checksum or crc16
	//  ***** You will have to write this simple function *****

    // TODO


#ifdef REPORT_INFO
	// block will be "r"ewritten
	COUT << "[r" << (int)(blkNum) << "]" << flush;
#endif
	sendLastByte(sendMostBlk(blkBufs[blkNum%2]));
}

//Send CAN_LEN copies of CAN characters in a row (in pairs spaced in time) to the
//  XMODEM receiver, to inform it of the cancelling of a file transfer.
//  There should be a total of (canPairs - 1) delays of
//  ((TM_2CHAR + TM_CHAR)/2 * mSECS_PER_UNIT) milliseconds
//  between the pairs of CAN characters.
void SenderX::can8()
{
	const int canBurst=2;
    char buffer[canBurst];
    memset( buffer, CAN, canBurst);

	const int canPairs=CAN_LEN/canBurst; 
	int x = 1;
	while (PE_NOT(myWrite(mediumD, buffer, canBurst), canBurst),
			x<canPairs) {
		++x;
	    this_thread::sleep_for (chrono::milliseconds(((TM_2CHAR + TM_CHAR)/2 * mSECS_PER_UNIT)));
	}
}

void SenderX::sendFile()
{
	transferringFileD = myOpen(fileName, O_RDONLY, 0);
	if(transferringFileD == -1 /* should be -1 */) {
		can8();
		COUT /* CERR */ << "Error opening input file named: " << fileName << endl;
		result = "OpenError";
	}
	else {	
		auto mySenderSmSp(make_shared<SenderSS>(this)); // or use make_unique
		mySenderSmSp->setDebugLog(NULL);

		// Because one source of sender behaviour is automatically generated by SmartState Studio,
		//	the behaviour has been factored out into a class SenderSS, but for your Part 2 submission
		//	we are not expecting a separate class as has been done here.
		while(mySenderSmSp->isRunning()) {
			unsigned char byteToReceive;
			PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
			mySenderSmSp->postEvent(SER, byteToReceive);
		}

		PE(myClose(transferringFileD));
		/*
		if (-1 == myClose(transferringFileD))
			VNS_ErrorPrinter("myClose(transferringFileD)", __func__, __FILE__, __LINE__, errno);
		*/
	}
}
