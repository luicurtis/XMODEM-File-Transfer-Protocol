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
// - TAs
// - Negar Bagheri Hariri
// - Darius Nadeem
// - Daniel Song
// - Victor Luz
//
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
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <iostream>
#include <stdint.h> // for uint8_t
#include <string.h> // for memset()
#include <fcntl.h>	// for O_RDONLY
#include <cstdlib>
#include <thread>
#include <stdlib.h>

#include "myIO.h"
#include "SenderX.h"
#include "VNPE.h"

using namespace std;

SenderX::SenderX(const char *fname, int d)
:PeerX(d, fname),
 bytesRd(-1),
 firstCrcBlk(true),
 blkNum(0)  	// but first block sent will be block #1, not #0
{
}

//-----------------------------------------------------------------------------

// get rid of any characters that may have arrived from the medium.
void SenderX::dumpGlitches()
{
	const int dumpBufSz = 20;
	char buf[dumpBufSz];
	int bytesRead;
	while (dumpBufSz == (bytesRead = PE(myReadcond(mediumD, buf, dumpBufSz, 0, 0, 0))));
}

// Send the block, less the block's last byte, to the receiver.
// Returns the block's last byte.

uint8_t SenderX::sendMostBlk(blkT blkBuf)
//uint8_t SenderX::sendMostBlk(uint8_t blkBuf[BLK_SZ_CRC])
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
	bytesRd = PE(read(transferringFileD, &blkBuf[DATA_POS], CHUNK_SZ ));
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
	genBlk(blkBufs[0]);     // Put the current block into [0]
}

/* refit the 1st block with a checksum
*/
void SenderX::cs1stBlk()
{
	// **** this function will need to be modified ****
    uint8_t cs = 0;
    // Calculate checksum
    for (int i = 3; i < 3+bytesRd; i++)
    {
        cs += blkBufs[0][i];
    }
    blkBufs[0][131] = cs;       // Store cs in the current block
}

/* while sending the now current block for the first time, prepare the next block if possible.
*/
void SenderX::sendBlkPrepNext()
{
	// **** this function will need to be modified ****
	blkNum ++; // 1st block about to be sent or previous block ACK'd
	uint8_t lastByte = sendMostBlk(blkBufs[0]);

	memcpy(blkBufs[1], blkBufs[0], BLK_SZ_CRC);       // store previous block into blkBufs[1]
	genBlk(blkBufs[0]); // prepare next block
	sendLastByte(lastByte);
}

// Resends the block that had been sent previously to the xmodem receiver
void SenderX::resendBlk()
{
	// resend the block including the checksum or crc16
	//  ***** You will have to write this simple function *****
    uint8_t lastByte = sendMostBlk( blkBufs[1]);
    sendLastByte(lastByte); //@sa: do we need to do this? sendBlkprepNext does it.
}

//Send CAN_LEN copies of CAN characters in a row (in pairs spaced in time) to the
//  XMODEM receiver, to inform it of the canceling of a file transfer.
//  There should be a total of (canPairs - 1) delays of
//  ((TM_2CHAR + TM_CHAR)/2 * mSECS_PER_UNIT) milliseconds
//  between the pairs of CAN characters.
void SenderX::can8()
{
	//  ***** You will have to write this simple function *****
	// use the C++11/14 standard library to generate the delays
    int wait_time_ms = ((TM_2CHAR + TM_CHAR)/2 * mSECS_PER_UNIT);
    for(int i = 0 ; i < CAN_LEN/2; i++)
    {
        sendByte(CAN);
        sendByte(CAN);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time_ms));
    }
}

void SenderX::sendFile()
{
	transferringFileD = myOpen(fileName, O_RDONLY, 0);
	if(transferringFileD == -1)
	{
		can8();
		cout /* cerr */ << "Error opening input file named: " << fileName << endl;
		result = "OpenError";
	}
	else
	{
		//blkNum = 0; // but first block sent will be block #1, not #0

		// ***** modify the below code according to the protocol *****
		// below is just a starting point.  You can follow a
		// 	different structure if you want.

        // Entry and Exit code for Sender_TopLevel
	    Crcflg = true;
	    prep1stBlk(); //@sa: modifies the bytesRd
        errCnt = 0;
        firstCrcBlk = true;

		char byteToReceive;

		PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);  // Read one byte that was received

		enum states{START, ACKNAK, EOT1, EOTEOT, CANCEL, EXIT} state = START;

		while (state != EXIT)
		{
		    switch(state)
		    {
                case START:
                    if((byteToReceive == NAK || byteToReceive == 'C') && bytesRd)
                    {
                        if (byteToReceive == NAK)
                        {
                            Crcflg = false;
                            cs1stBlk();
                            firstCrcBlk = false;
                        }
                        sendBlkPrepNext();
                        state = ACKNAK;
                    } // START -> EOT1
                    else if ((byteToReceive == NAK || byteToReceive == 'C') && !bytesRd)
                    {
                        if (byteToReceive == NAK)
                        {
                            firstCrcBlk = false;
                        }
                        sendByte(EOT);
                        state = EOT1;
                    }
                    else if(byteToReceive == NAK)
                    {
                        can8();
                        result = "ExcessiveNAKs";
                        state = EXIT;
                    }
                    else if(byteToReceive != NAK)
                    {
                        cerr<<"Sender received totally unexpected char #"<<byteToReceive<<": "<<(char)byteToReceive<<endl;
                        state = EXIT;
                        exit(EXIT_FAILURE);
                    }

                    break;
                case ACKNAK:
                    if((byteToReceive == ACK) && bytesRd)
                    {
                        sendBlkPrepNext();
                        errCnt = 0;
                        firstCrcBlk = false;
                        state = ACKNAK;
                    }
                    else if ((byteToReceive == NAK || ((byteToReceive =='C') && firstCrcBlk)) && (errCnt < errB))
                    {
                        resendBlk();
                        errCnt++;
                        state = ACKNAK;
                    }
                    else if (byteToReceive == CAN)
                    {
                        state = CANCEL;
                    }
                    else if ( (byteToReceive == ACK) && !bytesRd)
                    {
                        sendByte(EOT);
                        errCnt = 0;
                        firstCrcBlk = false;
                        state = EOT1;
                    }
                    else if(byteToReceive == NAK)
                    {
                        can8();
                        result = "ExcessiveNAKs";
                        state = EXIT;
                    }
                    else if(byteToReceive != NAK)
                    {
                        cerr<<"Sender received totally unexpected char #"<<byteToReceive<<": "<<(char)byteToReceive<<endl;
                        state = EXIT;
                        exit(EXIT_FAILURE);
                    }
                    break;
                case EOT1:
                    if(byteToReceive == NAK)
                    {
                        sendByte(EOT);
                        state = EOTEOT;
                    }
                    else if (byteToReceive == ACK)
                    {
                        result = "1st EOT ACK'd";
                        state = EXIT; //may need to change this
                    }
                    else if(byteToReceive == NAK)
                    {
                        can8();
                        result = "ExcessiveNAKs";
                        state = EXIT;
                    }
                    else if(byteToReceive != NAK)
                    {
                        cerr<<"Sender received totally unexpected char #"<<byteToReceive<<": "<<(char)byteToReceive<<endl;
                        state = EXIT;
                        exit(EXIT_FAILURE);
                    }
                    break;
                case EOTEOT:
                    if((byteToReceive == NAK) && (errCnt < errB))
                    {
                        sendByte(EOT);
                        errCnt++;
                        state = EOTEOT;
                    }
                    else if(byteToReceive == ACK)
                    {
                        result = "Done";
                        state = EXIT;
                    }
                    else if(byteToReceive == 'C')
                    {
                        can8();
                        result = "UnexpectedC";
                        state = EXIT;
                    }
                    else if(byteToReceive == NAK)
                    {
                        can8();
                        result = "ExcessiveNAKs";
                        state = EXIT;
                    }
                    else if(byteToReceive != NAK)
                    {
                        cerr<<"Sender received totally unexpected char #"<<byteToReceive<<": "<<(char)byteToReceive<<endl;
                        state = EXIT;
                        exit(EXIT_FAILURE);
                    }
                    break;
                case CANCEL:
                    if(byteToReceive == CAN)
                    {
                        result = "RcvCancelled";
                        state = EXIT;
                    }
                    else if(byteToReceive == NAK)
                    {
                        can8();
                        result = "ExcessiveNAKs";
                        state = EXIT;
                    }
                    else if(byteToReceive != NAK)
                    {
                        cerr<<"Sender received totally unexpected char #"<<byteToReceive<<": "<<(char)byteToReceive<<endl;
                        state = EXIT;
                        exit(EXIT_FAILURE);
                    }
                    break;

		    }//switch(state)

		    if (state != EXIT)
		    {
		        PE_NOT(myRead(mediumD, &byteToReceive, 1), 1);
		    }
		} // while (state != EXIT)

		PE(myClose(transferringFileD));

		/*
		if (-1 == myClose(transferringFileD))
			VNS_ErrorPrinter("myClose(transferringFileD)", __func__, __FILE__, __LINE__, errno);
		*/
	}
}

