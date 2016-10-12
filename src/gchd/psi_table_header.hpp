/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* This file in particular is for the encoding and decoding
 * of program specific information as specified by ISO/IEC 13818-1
 */
#ifndef PSI_TABLE_HEADER_H
#define PSI_TABLE_HEADER_H

#include <vector>
#include <cstdint>
#include "psi_data.hpp"

#define TABLE_ID_PAT 0x00 //Program association table.
#define TABLE_ID_PMT 0x02 //Program Map table.
#define TABLE_ID_SIT 0x7f //Selection information table.

class PSI_TableHeader : public PSI_Data
{
public:
    PSI_TableHeader(): tableID_(0xff), innerLength_(0), sectionSyntaxIndicator_(false), privateBit_(true) {};
    PSI_TableHeader( uint8_t tableID, bool sectionSyntaxIndicator=true, bool privateBit=true ): 
        tableID_(tableID), sectionSyntaxIndicator_(sectionSyntaxIndicator), privateBit_( privateBit ), innerLength_(0) {};

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size);

    uint8_t tableID_;
    uint16_t innerLength_; //this is sectionLength without the CRC.
    bool sectionSyntaxIndicator_; 
    bool privateBit_;
};

#endif
