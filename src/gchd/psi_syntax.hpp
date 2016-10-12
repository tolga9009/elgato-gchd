/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* This file in particular is for the encoding and decoding
 * of program specific information as specified by ISO/IEC 13818-1
 *
 * This is for the parsing of Syntax sections within a PSITable
 * This is an abstract class that you inherit from to create
 * parsers for PAT, PMT, and SIT information.
 */

#ifndef PSI_SYNTAX_H
#define PSI_SYNTAX_H

#include <vector>
#include <cstdint>
#include "psi_data.hpp"

//Empty for right now
class PSI_Syntax: public PSI_Data
{
public:    
    PSI_Syntax():
       extension_(0xffff), version_(0), currentIndicator_(true), sectionNumber_(0), lastSectionNumber_(0) {};
    PSI_Syntax(uint16_t extension, uint8_t version=0, bool currentIndicator=true, uint8_t sectionNumber=0, uint8_t lastSectionNumber=0):
       extension_(extension), version_(version), currentIndicator_(true), sectionNumber_(sectionNumber), lastSectionNumber_(lastSectionNumber) {};


    uint16_t extension_;
    uint8_t version_;
    bool currentIndicator_;
    uint8_t sectionNumber_;
    uint8_t lastSectionNumber_;

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size);
};

#endif


