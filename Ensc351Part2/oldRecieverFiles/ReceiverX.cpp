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
// File Name   : ReceiverX.cpp
// Version     : September 3rd, 2019
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include "myIO.h"
#include "ReceiverX.h"
#include "VNPE.h"

using namespace std;

ReceiverX::ReceiverX(int d, const char *fname, bool useCrc)
:PeerX(d, fname, useCrc),
NCGbyte(useCrc ? 'C' : NAK),
goodBlk(false), 
goodBlk1st(false), 
syncLoss(true),
numLastGoodBlk(0)
{
}

void ReceiverX::receiveFile()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	transferringFileD = PE2(myCreat(fileName, mode), fileName);

	// ***** improve this member function *****

	// below is just an example template.  You can follow a
	// 	different structure if you want.

	// inform sender that the receiver is ready and that the
	//		sender can send the first block
	sendByte(NCGbyte);

	while (PE_NOT(myRead(mediumD, rcvBlk, 1), 1) , (rcvBlk[0] == SOH)) // @cl: myRead(mediumD, rcvBlk, 1), 1) ONLY reads 1 byte once
	{
	    // Get rest of block
	    // Note: ALL checking will be done in getRestBlk()
	    getRestBlk();
        uint8_t blkNum_rcvd = rcvBlk[1];
        uint8_t blkNum_rcvd_lb = (numLastGoodBlk - 1 ) % 256;
        uint8_t blkNum_rcvd_ub = (numLastGoodBlk + 1 ) % 256;
        // Check #2: Check if we did received the previous block we just ACK'd
	    if (blkNum_rcvd == numLastGoodBlk ) //ERROR
	    {
	        // Set Flags
	        syncLoss = false;

            // @cl: Send ACK
            // @cl: DONT write chunk
            sendByte(ACK);
            goodBlk = true;         // Block received was good
            goodBlk1st = false;     // Set to false because it was a block we previously received
	    }
	    // Check #3: Check if blkNum received is off by more than 1
	    else if (!(blkNum_rcvd == numLastGoodBlk || blkNum_rcvd == blkNum_rcvd_ub || blkNum_rcvd ==blkNum_rcvd_lb) ) // *** CHECK WITH SINA ON
	    {
	        // Ignore checking
	        // Set Flags
	        goodBlk = false;
	        goodBlk1st = false;
	        syncLoss = true;
	        // send can8
	        can8();
	    }

        // Reply with ACK if the block received was good
        // Note:  once it goes into this if-statement, ALL the checking has been done and is good to write
	    else if(goodBlk)
		{
		    goodBlk1st = goodBlk = true;        // @cl: This is correct. getRestBlk() will only be called if
		                                        //      the received block passed all the checking
		    numLastGoodBlk = blkNum_rcvd;       // Set the last known good block number
		    sendByte(ACK);                      // Send ACK indicating we received a good block.
		    writeChunk();                       // Write the good block
		    goodBlk = false;                    // @cl: Set this back to false for the next block inc. ** check with TA
		}
		// Reply with NAK if block received was bad
		else
		{
		    sendByte(NAK);
		}

	} // while (PE_NOT(myRead(mediumD, rcvBlk, 1), 1) , (rcvBlk[0] == SOH))

	// End of Transfer
	// assume EOT was just read in the condition for the while loop
	sendByte(NAK); // NAK the first EOT

	//@sa : must make sure sender has send us an EOT byte
	//@cl: Keep looping until we get an EOT
	if (PE_NOT( myRead(mediumD, rcvBlk, 1), 1), (rcvBlk[0] != EOT))
	{
	    cerr<<"Receiver received totally unexpected char #" << endl;
        exit(EXIT_FAILURE);
	}
	else
	{
	    (close(transferringFileD));
	    // check if the file closed properly.  If not, result should be something other than "Done".
	    result = "Done"; //assume the file closed properly.
	    sendByte(ACK); // ACK the second EOT
	}

} // void ReceiverX::receiveFile()

/* Only called after an SOH character has been received.
The function tries
to receive the remaining characters to form a complete
block.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
*/
void ReceiverX::getRestBlk()
{
    // ********* this function must be improved ***********
    PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);   // @cl: this will get the entire rest of block and input into rcvBlk
//    goodBlk1st = goodBlk = true;    // @cl: Shouldn't be  in here

    // Start checking the block and set flags:
    // extracting the given info and now must test to see if they are consistent with our calculations
    uint8_t blkNum_rcvd = rcvBlk[1];
    uint8_t onesCmpl_rcvd = rcvBlk[2];

    // Check #0: Check complement and block number
    if(onesCmpl_rcvd == 255 - blkNum_rcvd){
        goodBlk = true;
    }
    // Check #1: Comparing calculated crc/chksum and
    if (goodBlk)
    {
        // Check the checksum
        if (NCGbyte == NAK)
        {
            uint8_t chksm_rcvd = rcvBlk[131];
            uint8_t sumP = 0;
            checksum(&sumP, rcvBlk);        // Function from PeerX.h

            // Compare checksum to chksm_rcvd and set: goodBlk, goodBlk1st, and syncLoss
            if (sumP != chksm_rcvd)
            {
                goodBlk = false;
                goodBlk1st = false;
                syncLoss = false;
            }
            else
            {
                goodBlk = true;
                goodBlk1st = true;
                syncLoss = false;
            }
        }
        else
        {
            // Extract CRC values
            uint16_t CRC_Hi = (uint16_t)rcvBlk[131] << 8;
            uint16_t CRC_Low = (uint16_t)rcvBlk[132];
            uint16_t crc_rcvd = CRC_Hi | CRC_Low;

            // Generate CRC
            uint16_t crcCalc;
            crc16ns(&crcCalc, &rcvBlk[DATA_POS]);       // Function from PeerX.h

            // Compare CRC to crc_rcvd and set: goodBlk, goodBlk1st, and syncLoss
            if (crcCalc != crc_rcvd)
            {
                // Set Flags
                goodBlk = false;
                goodBlk1st = false;
                syncLoss = false;
            }
            else
            {
               // Set Flags
               goodBlk = true;
               goodBlk1st = true;
               syncLoss = false;
            }
         }
    }
}

// Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

// Send 8 CAN characters in a row to the XMODEM sender, to inform it of
//	the canceling of a file transfer
void ReceiverX::can8()
{
	// no need to space CAN chars coming from receiver in time
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}
