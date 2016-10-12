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
 * selection information table (SIT).
 */

#ifndef PSI_SIT_H
#define PSI_SIT_H

#include <vector>
#include <memory>
#include <cstdint>
#include "psi_data.hpp"
#include "psi_table_header.hpp"
#include "psi_syntax.hpp"
#include "psi_descriptors.hpp"

//This is used to hold the internal data
//of a SIT. You probably want to use 
//SIT not SIT_Data if you want to create
//a SIT
class SIT_Data:public PSI_Data
{
public:
    SIT_Data(): transmissionParameters_(), services_() {};

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size ); //Abstract method.

    std::vector< std::shared_ptr<PSI_Descriptor> > transmissionParameters_;
    std::vector< std::shared_ptr<PSI_Descriptor> > services_;
};

class SIT:public PSI_Data
{
public:
    SIT():
        header_(TABLE_ID_SIT, true, true), syntaxSection_(0xffff), data_() {};

    PSI_TableHeader header_;
    PSI_Syntax syntaxSection_;
    SIT_Data data_;

    virtual int calculateSize();

    virtual void pack(std::vector<uint8_t> &outputData,
                      std::vector<uint8_t>::iterator &offset);

    virtual void unpack(const std::vector<uint8_t> &inputData,
                        std::vector<uint8_t>::const_iterator &offset,
                        int size );
};

#endif


