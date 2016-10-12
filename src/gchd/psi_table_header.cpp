/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* This file in particular is for the encoding and decoding
 * of program specific information as specified by ISO/IEC 13818-1
 */
#include "psi_table_header.hpp"
#include "psi_exceptions.hpp"
#include "../utility.hpp"
#include <iostream>
#include <sstream>

int PSI_TableHeader::calculateSize()
{
    return 3; //size of PsiTableHeader
}

void PSI_TableHeader::pack( std::vector<uint8_t> &outputData, std::vector<uint8_t>::iterator &offset )
{
    *offset++ = tableID_;

    uint16_t value=0;
    value |= sectionSyntaxIndicator_<<15;
    value |= privateBit_<<14;
    value |= 0x3 <<12; //reserved bits, set to 1.

    uint16_t sectionLength=innerLength_;
    if (sectionSyntaxIndicator_) {
        sectionLength += 4; //There will be a CRC added by hardware.
    }

    if( sectionLength > 1021 ) //standard dictates can't exceed this number
    {
        throw PSI_ValueException("A sectionLength value is set too high in a PSI table header.");
    }
    value |= sectionLength; 
    Utility::byteify<uint16_t>(&(*offset), value);

    offset+=2;
}

void PSI_TableHeader::unpack(const std::vector<uint8_t> &data,
                             std::vector<uint8_t>::const_iterator &offset,
                             int size)
{
    std::vector<uint8_t>::const_iterator endOffset=offset+size;

    const int headerSize=3;
    if(size < headerSize) {
        throw PSI_FormatException("Not enough space for even PSI table header.");
    }

    tableID_=*offset++;

    uint16_t value=Utility::debyteify<uint16_t>(&(*offset));
    offset+=2;

    sectionSyntaxIndicator_ = (value>>15) & 1;
    privateBit_ = (value>>14) & 1;
    uint16_t sectionLength = value & ((1<<10)-1);
    innerLength_=sectionLength;
    if( sectionSyntaxIndicator_ ) {
        innerLength_ -= 4; //There will be a CRC that HW will calculate.
    }
    
    //Check reserved and unused bits, his is strict checker that validates
    //we set up things correctly
    uint16_t reservedAndUnused = (value >> 10) & 0xf;
    if( reservedAndUnused != 0x3 ) {
        throw PSI_FormatException("Reserved/unused bits not set correctly in PSI table header.");
    }
    if( sectionLength > 1021 )  { //standard dictates can't exceed this number
        throw PSI_FormatException("Illegal sectionLength found in PSI table header.");
    }

    //The -4 is for the CRC, which we don't deal with because the hardware automatically
    //calculates it for us.
    if ((endOffset-offset) != innerLength_)
    {
        std::stringbuf outputString;
        std::ostream stream(NULL);
        stream.rdbuf(&outputString);
        stream << "The length of data after header: " << (endOffset-offset);
        stream << " is not equal to length decoded: " << innerLength_ << ".";
        throw PSI_FormatException( outputString.str() );
    }
}

         
