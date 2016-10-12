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
#include "psi_pat.hpp"
#include "../utility.hpp"

PAT_Entry::PAT_Entry( uint16_t programNumber, uint16_t pid)
{
	this->programNumber_=programNumber;
	this->pid_=pid;
	if( pid & ~((1<<13)-1) ) {
		//Totally invalid.
		throw PSI_ValueException( "Pid passed into PAT is larger than 13 bits.");
	}

	if ((pid < 0x0010) || (pid > 0x1ffe)) {
		throw PSI_ValueException( "Pid passed to PAT_Entry is in reserved range.");
	}
}

int PAT_Entry::calculateSize()
{
	return 4;
}

void PAT_Entry::pack(std::vector<uint8_t> &outputData,
		     std::vector<uint8_t>::iterator &offset)
{
	uint8_t *dataPointer = &(*offset);
	Utility::byteify<uint16_t>( dataPointer,  this->programNumber_ );
	offset+=2;

	uint16_t value = this->pid_;
	value |= 0x7<<13; //Top 3 bits are reserved.
	dataPointer = &(*offset);
	Utility::byteify<uint16_t>( dataPointer,  value );
	offset+=2;
}

void PAT_Entry::unpack( const std::vector<uint8_t> &inputData,
			std::vector<uint8_t>::const_iterator &offset,
			int size )
{
	if( size < 4) {
		throw PSI_FormatException("Cannot unpack PAT entry in less than 4 bytes.");
	}
	const uint8_t *dataPointer=&(*offset);
	std::vector<uint8_t>::const_iterator endOfDataOffset=offset+size;

	programNumber_=Utility::debyteify<uint16_t>( dataPointer );
	uint16_t value=Utility::debyteify<uint16_t>( dataPointer+2 );
	pid_ = value & ((1<<13)-1);
	//Only reason I check this is because the purpose of the unpack code
	//is to validate our own packing, not to decode actual streams.
	if ((value >> 13) != 0x7) {
		throw PSI_FormatException("Reserved bits in PAT entry are not set.");
	}
	offset+=4;
}

int PAT_Data::calculateSize()
{
	int length=0;
	for(unsigned i=0; i< entries_.size(); ++i)
	{
		length+=entries_[i].calculateSize();
	}
	return length;
}

void PAT_Data::pack(std::vector<uint8_t> &outputData,
		    std::vector<uint8_t>::iterator &offset)
{
	//Output entries.
	for(unsigned i=0; i<entries_.size(); ++i) {
		entries_[i].pack( outputData,
				  offset );
	}
}

void PAT_Data::unpack( const std::vector<uint8_t> &inputData,
		       std::vector<uint8_t>::const_iterator &offset,
		       int size )
{
	std::vector<uint8_t>::const_iterator endOffset=offset+size;

	///////////////////////////////////
	//Unpack entries.                //
	///////////////////////////////////
	//Reset transmissionParameters.
	entries_ = std::vector< PAT_Entry >();

	//Loop on transmission parameters.
	while(offset < endOffset) {
		PAT_Entry entry=PAT_Entry();
		entry.unpack(inputData, offset, endOffset-offset);
		entries_.push_back( entry );
	}
	if ( offset > endOffset ) {
		throw PSI_FormatException("PSI PAT size issue.");
	}
}

int PAT::calculateSize()
{
	int size;
	size=header_.calculateSize();
	size+=syntaxSection_.calculateSize();
	size+=data_.calculateSize();
	return size;
}

void PAT::pack(std::vector<uint8_t> &outputData,
	       std::vector<uint8_t>::iterator &offset)
{

	int syntaxSize=syntaxSection_.calculateSize();
	int dataSize=data_.calculateSize();

	header_.tableID_ = TABLE_ID_PAT;
	header_.sectionSyntaxIndicator_ = true;
	header_.privateBit_ = false;
	header_.innerLength_ = syntaxSize + dataSize; //sectionLength without CRC
	header_.pack( outputData, offset );

	//syntaxSection_.extension should already be set to the transport stream identifier
	//passed to the constructor.
	syntaxSection_.version_ = 0x0;
	syntaxSection_.currentIndicator_ = true;
	syntaxSection_.sectionNumber_ = 0;
	syntaxSection_.lastSectionNumber_ = 0;
	syntaxSection_.pack( outputData, offset );

	data_.pack( outputData, offset );
}

void PAT::unpack( const std::vector<uint8_t> &inputData,
		  std::vector<uint8_t>::const_iterator &offset,
		  int size )
{
	std::vector<uint8_t>::const_iterator endOffset=offset+size;
	header_.unpack( inputData, offset, endOffset-offset);

	if( header_.tableID_ != TABLE_ID_PAT ) {
		throw PSI_FormatException("PSI PAT table has wrong tableID.");
	}
	if( header_.sectionSyntaxIndicator_ != true ) {
		throw PSI_FormatException("PSI PAT table has wrong value for sectionSyntaxIndicator.");
	}
	if( header_.privateBit_ != false ) {
		throw PSI_FormatException("PSI PAT table has wrong value for private bit.");
	}

	syntaxSection_.unpack( inputData, offset, endOffset-offset );

	if( syntaxSection_.version_ != 0) {
		throw PSI_FormatException("PSI PAT table has version other than 0.");
	}
	if( syntaxSection_.currentIndicator_ != true ) {
		throw PSI_FormatException("PSI PAT has current not set to true.");
	}
	if( syntaxSection_.sectionNumber_ != 0 ) {
		throw PSI_FormatException("PSI PAT has section number not set to 0.");
	}
	if( syntaxSection_.lastSectionNumber_ != 0 ) {
		throw PSI_FormatException("PSI PAT has last section number not set to 0.");
	}

	data_.unpack( inputData, offset, endOffset-offset );
	if( offset != endOffset ) {
		throw PSI_FormatException("PSI PAT passed in is the wrong size.");
	}
}

void PAT::clearEntries()
{
	this->data_.entries_.resize(0);
}

void PAT::addEntry(PAT_Entry value)
{
	this->data_.entries_.push_back( value );
}

std::vector<PAT_Entry> *PAT::getEntries()
{
	return &this->data_.entries_;
}

uint16_t PAT::getTransportStreamIdentifier()
{
	return syntaxSection_.extension_;
}


