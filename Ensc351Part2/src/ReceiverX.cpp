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

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include "myIO.h"
#include "ReceiverX.h"
#include "VNPE.h"
#include <cstdlib>

using namespace std;

ReceiverX::
ReceiverX(int d, const char *fname, bool useCrc)
:PeerX(d, fname, useCrc), 
NCGbyte(useCrc ? 'C' : NAK),
goodBlk(false), 
goodBlk1st(false), 
syncLoss(false), // @cl: previously true
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

	while(PE_NOT(myRead(mediumD, rcvBlk, 1), 1), (rcvBlk[0] == SOH))
	{
		getRestBlk();
	}

	// End of Transfer
	if(rcvBlk[0] == EOT)
	{
	    sendByte(NAK);
	    if (PE_NOT( myRead(mediumD, rcvBlk, 1), 1), (rcvBlk[0] == EOT))
	    {
	        result = "Done"; //assume the file closed properly.
	        sendByte(ACK); // ACK the second EOT
	        myClose(transferringFileD);
	    }
	    else
	    {
	        cerr<<"Receiver received totally unexpected char #" << endl;
	        exit(EXIT_FAILURE);
	    }
	}
	else if(rcvBlk[0] == CAN)
	{
	    result = "SenderCancelled";
        if (PE_NOT( myRead(mediumD, rcvBlk, 1), 1), (rcvBlk[0] == CAN)){
            myClose(transferringFileD);
        }
        else
        {
            cerr<<"Receiver received totally unexpected char #" << endl;
            exit(EXIT_FAILURE);
        }

	}
	else
	{
        cerr<<"Receiver received totally unexpected char #" << endl;
        exit(EXIT_FAILURE);
	}
}

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
    if(NCGbyte == NAK){
        PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CS, REST_BLK_SZ_CS, 0, 0), REST_BLK_SZ_CS);
    }
    else
    {
        PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);
    }
	// goodBlk1st = goodBlk = true;
	checkandReply();
}

//Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

//Send 8 CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverX::can8()
{
	// no need to space CAN chars coming from receiver in time
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}

void ReceiverX::checkandReply(){
	
	enum FAILURE_STATE{SYNCLOSS, BLKNUM, ONES_BLKNUM, CHCKSUM_CRC, ERRCNT, PASS} failureState = PASS;

	//SYNCLOSS TEST paramters
	uint8_t blkNum_rcvd    = rcvBlk[1];
	uint8_t blkNum_rcvd_lb = (numLastGoodBlk - 1 ) % 256;
    uint8_t blkNum_rcvd_ub = (numLastGoodBlk + 1 ) % 256;
	
    // Test#0 : testing blkNum
	if (rcvBlk[1] == numLastGoodBlk)
	{
		failureState = BLKNUM;
	}
	// Test#1: Testing sync loss
	else if (!(blkNum_rcvd == numLastGoodBlk || blkNum_rcvd == blkNum_rcvd_ub || blkNum_rcvd ==blkNum_rcvd_lb))
	{	
		failureState = SYNCLOSS;
	}
	// Test#2: testing ones complement of blk num
	else if(rcvBlk[2] != 255 - rcvBlk[1])
	{
		failureState = ONES_BLKNUM;
	}
	else if(errCnt >= errB)
	{
	    failureState = ERRCNT;
	}
	// Test#3: Test CRC and Checksum
	if(failureState == PASS)
	{
        if(NCGbyte == NAK)
        {
            uint8_t chksm_rcvd = rcvBlk[131];
            uint8_t sumP = 0;
            checksum(&sumP, rcvBlk);        // Function from PeerX.h
            if(sumP != chksm_rcvd)
            {
                failureState = CHCKSUM_CRC;
            }
        }
        else //crc test
        {
            uint16_t crcCalc = 0;
            crc16ns((uint16_t*)&crcCalc, &rcvBlk[DATA_POS]);
            if((uint16_t)crcCalc != *(uint16_t*)&rcvBlk[131])
            {
                failureState = CHCKSUM_CRC;
            }
            //if (crcCalc != crc_rcvd){ failureState = CHCKSUM_CRC;}
        }
	}

	switch(failureState)
	{
		case(SYNCLOSS):
		{
			goodBlk = false;
	        goodBlk1st = false;
	        syncLoss = true;
	        result = "syncLoss";
			can8();
			break;
		}
		case(BLKNUM):
		{
		    // sendACK, dont write, increment errCnt
			goodBlk = false;
			sendByte(ACK);
			errCnt++;
			break;
		}
		case(ONES_BLKNUM):
		{
			goodBlk = false;
			sendByte(NAK);
			errCnt++;
			break;
		}
		case(ERRCNT):
        {
		    result= "errCnt >= errB";
		    can8();
            break;
        }
		case(CHCKSUM_CRC):
		{
			goodBlk = false;
			goodBlk1st = false;
			syncLoss = false;
			sendByte(NAK);
			errCnt++;
			break;
		}
		case(PASS):
		{
			goodBlk1st = true;
			goodBlk = false;
			numLastGoodBlk = rcvBlk[1];
			errCnt = 0;
			sendByte(ACK);
			writeChunk();
			break;
		}
	}
}



