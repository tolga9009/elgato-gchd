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
 * program map table (PMT).
 */

#ifndef PSI_PMT_H
#define PSI_PMT_H

#include <vector>
#include <memory>
#include <cstdint>
#include "psi_data.hpp"
#include "psi_table_header.hpp"
#include "psi_syntax.hpp"
#include "psi_descriptors.hpp"

//This is used to hold the internal data
//of a PMT. You probably want to use 
//PMT not PMT_Data if you want to create
//a PMT


// https://en.wikipedia.org/wiki/Program-specific_information#Elementary_stream_types
#define STREAM_TYPE_AAC  0x0f
#define STREAM_TYPE_H264 0x1b

class PMT_Mapping:public PSI_Data
{
public:
    //Always use this constructor. Set descriptors after construction.
    PMT_Mapping(uint16_t elementaryPid, uint8_t streamType=STREAM_TYPE_H264);

    //This default constructor shouldn't be used except by unpacking,
    //and by stl for creation of entries..
    PMT_Mapping(): elementaryPid_(0), streamType_(0), descriptors_() {}; //0xf illegal value on purpose.

    uint16_t elementaryPid_;
    uint16_t streamType_;
    std::vector<std::shared_ptr<PSI_Descriptor>> descriptors_;    

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size ); //Abstract method.

    virtual void clearDescriptors();
    virtual void addDescriptor( std::shared_ptr<PSI_Descriptor> descriptor );

    //Pointer is invalid if PMT_entry instance is destroyed.
    virtual std::vector<std::shared_ptr<PSI_Descriptor>> *getDescriptors(); 
};

class PMT_Data:public PSI_Data
{
public:
    PMT_Data(uint16_t pcrPid): pcrPid_(pcrPid), programInfo_(), mapEntries_() {};
    PMT_Data(): pcrPid_(0x1fff), programInfo_(), mapEntries_() {};

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size ); //Abstract method.

    uint16_t pcrPid_;
    std::vector< std::shared_ptr<PSI_Descriptor> > programInfo_;
    std::vector< PMT_Mapping > mapEntries_;
};

class PMT:public PSI_Data
{
public:
    PMT(uint16_t programNumber, uint16_t pcrPid):
        header_(TABLE_ID_PMT, true, false), syntaxSection_(programNumber), data_(pcrPid) {};

    PSI_TableHeader header_;
    PSI_Syntax syntaxSection_;
    PMT_Data data_;

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size );

    virtual void clearProgramInfo();
    virtual void addProgramInfo( std::shared_ptr<PSI_Descriptor> descriptor );

    //Pointer is invalid if PAT instance is destroyed.
    virtual std::vector<std::shared_ptr<PSI_Descriptor>> *getProgramInfo(); 

    virtual void clearMapEntries();
    virtual void addMapEntry( PMT_Mapping mapping );

    //Pointer is invalid if PAT instance is destroyed.
    virtual std::vector<PMT_Mapping> *getMapEntries(); 
 
    uint16_t getProgramNumber();
    uint16_t getPcrPid();
};

#endif


