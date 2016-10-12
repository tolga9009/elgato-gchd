/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* This file is for the encoding and decoding of the syntax sections
 * of program specific information as specified by ISO/IEC 13818-1
 */
#include "psi_syntax.hpp"
#include "psi_exceptions.hpp"
#include "psi_sit.hpp"
#include "../utility.hpp"

int SIT_Data::calculateSize()
{
    int length=2; //baseline length
    for(unsigned i=0; i< transmissionParameters_.size(); ++i)
    {
        length+=transmissionParameters_[i]->calculateSize();
    }
    for(unsigned i=0; i<services_.size(); ++i)
    {
        length+=services_[i]->calculateSize();
    }
    return length;
}

void SIT_Data::pack(std::vector<uint8_t> &outputData,
                    std::vector<uint8_t>::iterator &offset) 
{
    int transmissionInfoLoopLength=0;
    for(unsigned i=0; i<transmissionParameters_.size(); ++i) {
        transmissionInfoLoopLength+=transmissionParameters_[i]->calculateSize();
    }
    if (transmissionInfoLoopLength > (1<<12)-1) {
        throw PSI_ValueException("PSI SIT table has too many transmission info parameters.");
    }
    uint16_t outputValue=(0xf<<12) | transmissionInfoLoopLength;

    uint8_t *dataPointer = &(*offset);
    Utility::byteify<uint16_t>( dataPointer,  outputValue );
    offset+=2;
    
    //Output transmission parameters.
    for(unsigned i=0; i<transmissionParameters_.size(); ++i) {
        transmissionParameters_[i]->pack( outputData,
                                        offset );
    }
    //Output services                                      
    for(unsigned i=0; i<services_.size(); ++i) {
        services_[i]->pack( outputData,
                            offset );
    }
}
                          
void SIT_Data::unpack( const std::vector<uint8_t> &inputData,
                       std::vector<uint8_t>::const_iterator &offset,
                       int size ) 
{
    int overhead=2; //2 byte transmissionInfoLoopLength 
    if (size < overhead) {
        throw PSI_FormatException("PSI SIT table needs to be at least 2 bytes.");
    }

    const uint8_t *dataPointer=&(*offset);  
    std::vector<uint8_t>::const_iterator endOfDataOffset=offset+size;

    uint16_t value=Utility::debyteify<uint16_t>( dataPointer );

    //This is a strict checker used to check itself, so
    //we don't ignore reserved bits.
    if(( value >> 12 ) != 0xf) {
        throw PSI_FormatException("PSI SIT reserved bits not set correctly.");
    }
    uint16_t transmissionInfoLoopLength= value & ((1<<12)-1);

    ///////////////////////////////////
    //Unpack transmission Parameters.//
    ///////////////////////////////////
    offset+=2;
    std::vector<uint8_t>::const_iterator endTransmissionParametersOffset=offset+transmissionInfoLoopLength;
    if (endTransmissionParametersOffset > endOfDataOffset) {
        throw PSI_FormatException("PSI SIT size issue."); 
    }

    //Reset transmissionParameters.
    transmissionParameters_ = std::vector< std::shared_ptr<PSI_Descriptor >>();

    //Loop on transmission parameters.
    while(offset < endTransmissionParametersOffset) {
        auto transmissionParameter=std::make_shared<PSI_ParseDescriptor>();
        transmissionParameter->unpack(inputData, offset, endTransmissionParametersOffset-offset);
        transmissionParameters_.push_back( transmissionParameter->getParsedDescriptor() );
    }
    if ( offset != endTransmissionParametersOffset ) {
        throw PSI_FormatException("PSI SIT size issue."); 
    }

    ////////////////////
    //Unpack Services.//
    ////////////////////
    //Reset services_
    services_ = std::vector< std::shared_ptr<PSI_Descriptor >>();

    while(offset < endOfDataOffset) {
        auto service=std::make_shared<PSI_ParseDescriptor>();
        service->unpack(inputData, offset, endOfDataOffset-offset); 
        services_.push_back( service->getParsedDescriptor() );
    }
    if ( offset > endOfDataOffset ) {
        throw PSI_FormatException("PSI SIT size issue."); 
    }
}

int SIT::calculateSize()
{
    int size;
    size=header_.calculateSize();
    size+=syntaxSection_.calculateSize();
    size+=data_.calculateSize();
    return size;
}

void SIT::pack(std::vector<uint8_t> &outputData,
               std::vector<uint8_t>::iterator &offset)
{

    int syntaxSize=syntaxSection_.calculateSize();
    int dataSize=data_.calculateSize();

    header_.tableID_ = TABLE_ID_SIT;
    header_.sectionSyntaxIndicator_ = true;
    header_.privateBit_ = true;
    header_.innerLength_ = syntaxSize + dataSize; //sectionLength without CRC
    header_.pack( outputData, offset );

    syntaxSection_.extension_ = 0xffff;
    syntaxSection_.version_ = 0x0;
    syntaxSection_.currentIndicator_ = true;
    syntaxSection_.sectionNumber_ = 0;
    syntaxSection_.lastSectionNumber_ = 0;
    syntaxSection_.pack( outputData, offset );

    data_.pack( outputData, offset );
}

void SIT::unpack( const std::vector<uint8_t> &inputData,
                  std::vector<uint8_t>::const_iterator &offset,
                  int size ) 
{
    std::vector<uint8_t>::const_iterator endOffset=offset+size;
    header_.unpack( inputData, offset, endOffset-offset);

    if( header_.tableID_ != TABLE_ID_SIT ) {
        throw PSI_FormatException("PSI SIT table has wrong tableID.");
    }
    if( header_.sectionSyntaxIndicator_ != true ) {
        throw PSI_FormatException("PSI SIT table has wrong value for sectionSyntaxIndicator.");
    }
    if( header_.privateBit_ != true ) {
        throw PSI_FormatException("PSI SIT table has wrong value for private bit.");
    }

    syntaxSection_.unpack( inputData, offset, endOffset-offset );

    if( syntaxSection_.extension_ !=0xffff) {
        //Super strict checker, check reserved bits.
        throw PSI_FormatException("PSI SIT table has extension reserved bits set to something other than 0xffff.");
    }
    if( syntaxSection_.version_ != 0) {
        throw PSI_FormatException("PSI SIT table has version other than 0.");
    }
    if( syntaxSection_.currentIndicator_ != true ) {
        throw PSI_FormatException("PSI SIT has current not set to true.");
    }
    if( syntaxSection_.sectionNumber_ != 0 ) {
        throw PSI_FormatException("PSI SIT has section number not set to 0.");
    }
    if( syntaxSection_.lastSectionNumber_ != 0 ) {
        throw PSI_FormatException("PSI SIT has last section number not set to 0.");
    }
 
    data_.unpack( inputData, offset, endOffset-offset );
    if( offset != endOffset ) {
        throw PSI_FormatException("PSI SIT passed in is the wrong size.");
    }
}

  
