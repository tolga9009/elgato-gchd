/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* This file in particular is for the encoding and decoding
 * of program specific information as specified by ISO/IEC 13818-1
 *
 * This file in particular is for the encoding/decoding of the
 * program association table (PAT).
 */

#ifndef PSI_PAT_H
#define PSI_PAT_H

#include <vector>
#include <cstdint>
#include "psi_data.hpp"
#include "psi_table_header.hpp"
#include "psi_syntax.hpp"

class PAT_Entry:public PSI_Data
{
public:
    //Always use this constructor.
    PAT_Entry(uint16_t programNumber, uint16_t pid);

    //This default constructor shouldn't be used except by unpacking,
    //and by stl for creation of entries..
    PAT_Entry(): programNumber_(0), pid_(0xf) {}; //0xf illegal value on purpose.

    uint16_t programNumber_;
    uint16_t pid_;

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size ); //Abstract method.

};

//This is used to hold the internal data
//of a PAT. You probably want to use 
//PAT not PAT_Data if you want to create
//a PAT
class PAT_Data:public PSI_Data
{
public:
    PAT_Data(): entries_() {};

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size ); //Abstract method.

    std::vector< PAT_Entry > entries_;
};

class PAT:public PSI_Data
{
public:
    PAT(uint16_t transportStreamIdentifier):
        header_(TABLE_ID_PAT, true, false), syntaxSection_(transportStreamIdentifier), data_() {};

    PSI_TableHeader header_;
    PSI_Syntax syntaxSection_;
    PAT_Data data_;

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size );

    virtual void clearEntries();
    virtual void addEntry( PAT_Entry entry );

    //Pointer is invalid if PAT instance is destroyed.
    virtual std::vector<PAT_Entry> *getEntries(); 
    uint16_t getTransportStreamIdentifier();
};

#endif


