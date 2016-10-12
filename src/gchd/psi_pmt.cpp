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

#include "psi_syntax.hpp"
#include "psi_exceptions.hpp"
#include "psi_pmt.hpp"
#include "../utility.hpp"

PMT_Mapping::PMT_Mapping( uint16_t elementaryPid, uint8_t streamType)
{
	this->elementaryPid_=elementaryPid;
	this->streamType_=streamType;
	if( elementaryPid & ~((1<<13)-1) ) {
		//Totally invalid.
		throw PSI_ValueException( "Pid passed into PMT_Mapping is larger than 13 bits.");
	}
	this->descriptors_=std::vector<std::shared_ptr<PSI_Descriptor>>();
}

int PMT_Mapping::calculateSize()
{
	unsigned length=5;
	for( unsigned i=0; i< descriptors_.size(); ++i)
	{
		length += descriptors_[i]->calculateSize();
	}
	return length;
}

void PMT_Mapping::pack(std::vector<uint8_t> &outputData,
		       std::vector<uint8_t>::iterator &offset)
{
	uint8_t *dataPointer = &(*offset);
	*dataPointer++=streamType_;
	offset+=1;
	if( elementaryPid_ >= 1<<13 ) {
		throw PSI_ValueException("elementaryPID in PMT_Mapping has value bigger than 13 bits.");
	}
	uint16_t value=(0x7<<13) | elementaryPid_;
	Utility::byteify<uint16_t>( dataPointer,  value );
	dataPointer+=2;
	offset+=2;

	unsigned descriptorLength=0;
	for( unsigned i=0; i< descriptors_.size(); ++i) {
		descriptorLength += descriptors_[i]->calculateSize();
	}
	if( descriptorLength >= (1<<10) ) {
		throw PSI_ValueException("ES_info_length in PMT_Mapping too big. Total size of descriptors is too big.");
	}

	value = (0xf<<12) | descriptorLength;
	Utility::byteify<uint16_t>( dataPointer, value );
	dataPointer+=2;
	offset+=2;

	for( unsigned i=0; i< descriptors_.size(); ++i) {
		descriptors_[i]->pack( outputData, offset );
	}
}

void PMT_Mapping::unpack( const std::vector<uint8_t> &inputData,
			  std::vector<uint8_t>::const_iterator &offset,
			  int size )
{
	if( size < 5) {
		throw PSI_FormatException("Cannot unpack PMT_Mapping entry in less than 5 bytes.");
	}
	std::vector<uint8_t>::const_iterator endOfDataOffset=offset+size;

	const uint8_t *dataPointer=&(*offset);
	streamType_ = *dataPointer++;
	offset+=1;

	uint16_t value=Utility::debyteify<uint16_t>( dataPointer );
	elementaryPid_ = value & ((1<<13)-1);

	//Only reason I check this is because the purpose of the unpack code
	//is to validate our own packing, not to decode actual streams.
	if ((value >> 13) != 0x7) {
		throw PSI_FormatException("Reserved bits in PMT_Mapping are not set.");
	}
	dataPointer+=2;
	offset+=2;

	value=Utility::debyteify<uint16_t>( dataPointer );
	uint16_t es_info_length = value & ((1<<10)-1);

	if ((value >> 10) != 0x3c) { //4 set bits, followed by 2 clear bits.
		throw PSI_FormatException("Reserved bits in PMT_Mapping are not set.");
	}
	dataPointer+=2;
	offset +=2;
	std::vector<uint8_t>::const_iterator endOfDescriptors = offset + es_info_length;

	//Reset descriptors__
	descriptors_ = std::vector< std::shared_ptr<PSI_Descriptor> >();

	while(offset < endOfDescriptors) {
		auto descriptor=std::shared_ptr<PSI_ParseDescriptor>();
		descriptor->unpack(inputData, offset, endOfDescriptors-offset);
		descriptors_.push_back( descriptor->getParsedDescriptor() );
	}
	if ( offset > endOfDescriptors ) {
		throw PSI_FormatException("PSI PMT_Mapping size issue.");
	}
}

void PMT_Mapping::clearDescriptors(void)
{
	descriptors_ = std::vector< std::shared_ptr<PSI_Descriptor> >();
}

void PMT_Mapping::addDescriptor( std::shared_ptr<PSI_Descriptor> descriptor )
{
	descriptors_.push_back( descriptor );
}


//Pointer is invalid if PMT_Mapping instance is destroyed.
std::vector<std::shared_ptr<PSI_Descriptor>> *PMT_Mapping::getDescriptors()
{
	return &descriptors_;
}

int PMT_Data::calculateSize()
{
	int length=4;
	for(unsigned i=0; i< programInfo_.size(); ++i) {
		length += programInfo_[i]->calculateSize();
	}
	for(unsigned i=0; i< mapEntries_.size(); ++i) {
		length += mapEntries_[i].calculateSize();
	}
	return length;
}

void PMT_Data::pack(std::vector<uint8_t> &outputData,
		    std::vector<uint8_t>::iterator &offset)
{
	if( pcrPid_ >= (1<<13) ) {
		throw PSI_ValueException("PCR_PID value in PMT is bigger than 13 bits.");
	}
	uint16_t value=(0x7<<13) | pcrPid_;
	Utility::byteify<uint16_t>( &(*offset), value );
	offset+=2;

	uint16_t programInfoLength=0;
	for(unsigned i=0; i< programInfo_.size(); ++i) {
		programInfoLength+=programInfo_[i]->calculateSize();
	}
	if( programInfoLength >= (1<<10)) {
		throw PSI_ValueException("program_info_length in PMT_Data is larger than 10 bits.");
	}
	value=programInfoLength | (0xf<<12);
	Utility::byteify<uint16_t>( &(*offset), value );
	offset+=2;

	//Output programInfo.
	for(unsigned i=0; i<programInfo_.size(); ++i) {
		programInfo_[i]->pack( outputData,
				       offset );
	}
	//Output mapEntries.
	for(unsigned i=0; i<mapEntries_.size(); ++i) {
		mapEntries_[i].pack( outputData,
				     offset );
	}
}

void PMT_Data::unpack( const std::vector<uint8_t> &inputData,
		       std::vector<uint8_t>::const_iterator &offset,
		       int size )
{
	int overhead=4; //4 byte overhead
	if (size < overhead) {
		throw PSI_FormatException("PSI PMT table needs to be at least 4 bytes.");
	}

	std::vector<uint8_t>::const_iterator endOffset=offset+size;

	uint16_t value=Utility::debyteify<uint16_t>( &(*offset) );
	offset+=2;
	if(( value >> 13 ) != 7) {
		throw PSI_FormatException("PSI PMT reserved bits not set correctly.");
	}
	pcrPid_=value & ((1<<13)-1);

	value=Utility::debyteify<uint16_t>( &(*offset) );
	offset+=2;
	//This is a strict checker used to check itself, so
	//we don't ignore reserved bits.
	if(( value >> 10 ) != 0x3c) {
		throw PSI_FormatException("PSI PMT reserved bits not set correctly.");
	}
	uint16_t programInfoLength=value & ((1<<10)-1);
	std::vector<uint8_t>::const_iterator endInfoOffset = offset + programInfoLength;

	if( endInfoOffset > endOffset ) {
		throw PSI_FormatException("PSI PMT program info length longer than rest of PMT.");
	}

	///////////////////////////////////
	//Unpack program info            //
	///////////////////////////////////
	//Reset transmissionParameters.
	programInfo_ = std::vector< std::shared_ptr<PSI_Descriptor> >();

	//Loop on transmission parameters.
	while(offset < endInfoOffset) {
		auto descriptor=std::make_shared<PSI_ParseDescriptor>();
		descriptor->unpack(inputData, offset, endInfoOffset-offset);
		programInfo_.push_back( descriptor->getParsedDescriptor() );
	}
	if (offset != endInfoOffset) {
		throw PSI_FormatException("PSI PMT size issue.");
	}

	///////////////////////////////////
	//Unpack map entries             //
	///////////////////////////////////

	mapEntries_ = std::vector<PMT_Mapping>();
	while(offset < endOffset) {
		PMT_Mapping mapping=PMT_Mapping();
		mapping.unpack(inputData, offset, endOffset-offset);
		mapEntries_.push_back( mapping );
	}
	if (offset != endOffset) {
		throw PSI_FormatException("PSI PMT size issue.");
	}
}

int PMT::calculateSize()
{
	int size;
	size=header_.calculateSize();
	size+=syntaxSection_.calculateSize();
	size+=data_.calculateSize();
	return size;
}

void PMT::pack(std::vector<uint8_t> &outputData,
	       std::vector<uint8_t>::iterator &offset)
{

	int syntaxSize=syntaxSection_.calculateSize();
	int dataSize=data_.calculateSize();

	header_.tableID_ = TABLE_ID_PMT;
	header_.sectionSyntaxIndicator_ = true;
	header_.privateBit_ = false;
	header_.innerLength_ = syntaxSize + dataSize; //sectionLength without CRC
	header_.pack( outputData, offset );

	//syntaxSection_.extension should already be set to progrmaNumber
	//passed to the constructor.
	syntaxSection_.version_ = 0x0;
	syntaxSection_.currentIndicator_ = true;
	syntaxSection_.sectionNumber_ = 0;
	syntaxSection_.lastSectionNumber_ = 0;
	syntaxSection_.pack( outputData, offset );

	data_.pack( outputData, offset );
}

void PMT::unpack( const std::vector<uint8_t> &inputData,
		  std::vector<uint8_t>::const_iterator &offset,
		  int size )
{
	std::vector<uint8_t>::const_iterator endOffset=offset+size;
	header_.unpack( inputData, offset, endOffset-offset);

	if( header_.tableID_ != TABLE_ID_PMT ) {
		throw PSI_FormatException("PSI PMT table has wrong tableID.");
	}
	if( header_.sectionSyntaxIndicator_ != true ) {
		throw PSI_FormatException("PSI PMT table has wrong value for sectionSyntaxIndicator.");
	}
	if( header_.privateBit_ != false ) {
		throw PSI_FormatException("PSI PMT table has wrong value for private bit.");
	}

	syntaxSection_.unpack( inputData, offset, endOffset-offset );

	if( syntaxSection_.version_ != 0) {
		throw PSI_FormatException("PSI PMT table has version other than 0.");
	}
	if( syntaxSection_.currentIndicator_ != true ) {
		throw PSI_FormatException("PSI PMT has current not set to true.");
	}
	if( syntaxSection_.sectionNumber_ != 0 ) {
		throw PSI_FormatException("PSI PMT has section number not set to 0.");
	}
	if( syntaxSection_.lastSectionNumber_ != 0 ) {
		throw PSI_FormatException("PSI PMT has last section number not set to 0.");
	}

	data_.unpack( inputData, offset, endOffset-offset );
	if( offset != endOffset ) {
		throw PSI_FormatException("PSI PMT passed in is the wrong size.");
	}
}

void PMT::clearProgramInfo()
{
	this->data_.programInfo_.resize(0);
}

void PMT::addProgramInfo(std::shared_ptr<PSI_Descriptor> descriptor)
{
	this->data_.programInfo_.push_back( descriptor );
}

std::vector<std::shared_ptr<PSI_Descriptor>> *PMT::getProgramInfo()
{
	return &this->data_.programInfo_;
}

void PMT::clearMapEntries()
{
	this->data_.mapEntries_.resize(0);
}

void PMT::addMapEntry(PMT_Mapping mapping)
{
	this->data_.mapEntries_.push_back( mapping );
}

std::vector<PMT_Mapping> *PMT::getMapEntries()
{
	return &this->data_.mapEntries_;
}

uint16_t PMT::getProgramNumber()
{
	return syntaxSection_.extension_;
}

uint16_t PMT::getPcrPid()
{
	return data_.pcrPid_;
}


