///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright � NetworkDLS 2010, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NSWFL_CRC32_CPP_
#define _NSWFL_CRC32_CPP_
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "nswfl_crc32.h"
#include <memory>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace taflib;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    This function initializes "CRC Lookup Table". You only need to call it once to
        initalize the table before using any of the other CRC32 calculation functions.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CRC32::CRC32(void)
{
    this->Initialize();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CRC32::~CRC32(void)
{
    //No destructor code.
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    This function initializes "CRC Lookup Table". You only need to call it once to
        initalize the table before using any of the other CRC32 calculation functions.
*/

void CRC32::Initialize(void)
{
    //0x04C11DB7 is the official polynomial used by PKZip, WinZip and Ethernet.
    unsigned int iPolynomial = 0x04C11DB7;

    memset(&this->iTable, 0, sizeof(this->iTable));

    // 256 values representing ASCII character codes.
    for (int iCodes = 0; iCodes <= 0xFF; iCodes++)
    {
        this->iTable[iCodes] = this->Reflect(iCodes, 8) << 24;

        for (int iPos = 0; iPos < 8; iPos++)
        {
            this->iTable[iCodes] = (this->iTable[iCodes] << 1)
                ^ ((this->iTable[iCodes] & (1 << 31)) ? iPolynomial : 0);
        }

        this->iTable[iCodes] = this->Reflect(this->iTable[iCodes], 32);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    Reflection is a requirement for the official CRC-32 standard.
    You can create CRCs without it, but they won't conform to the standard.
*/

unsigned int CRC32::Reflect(unsigned int iReflect, const char cChar)
{
    unsigned int iValue = 0;

    // Swap bit 0 for bit 7, bit 1 For bit 6, etc....
    for (int iPos = 1; iPos < (cChar + 1); iPos++)
    {
        if (iReflect & 1)
        {
            iValue |= (1 << (cChar - iPos));
        }
        iReflect >>= 1;
    }

    return iValue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    Calculates the CRC32 by looping through each of the bytes in sData.

    Note: For Example usage example, see FileCRC().
*/

void CRC32::PartialCRC(unsigned int *iCRC, const unsigned char *sData, size_t iDataLength) const
{
    while (iDataLength--)
    {
        //If your compiler complains about the following line, try changing
        //	each occurrence of *iCRC with ((unsigned int)*iCRC).

        *iCRC = (*iCRC >> 8) ^ this->iTable[(*iCRC & 0xFF) ^ *sData++];
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    Returns the calculated CRC32 (through iOutCRC) for the given string.
*/

void CRC32::FullCRC(const unsigned char *sData, size_t iDataLength, unsigned int *iOutCRC) const
{
    *iOutCRC = 0xffffffff; //Initilaize the CRC.

    this->PartialCRC(iOutCRC, sData, iDataLength);

    *iOutCRC ^= 0xffffffff; //Finalize the CRC.
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    Returns the calculated CRC23 for the given string.
*/

unsigned int CRC32::FullCRC(const unsigned char *sData, size_t iDataLength)
{
    unsigned int iCRC = 0xffffffff; //Initilaize the CRC.

    this->PartialCRC(&iCRC, sData, iDataLength);

    return(iCRC ^ 0xffffffff); //Finalize the CRC and return.
}

#endif
