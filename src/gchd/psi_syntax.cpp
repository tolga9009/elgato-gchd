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
#include "../utility.hpp"

int PSI_Syntax::calculateSize() {
	return 5; //5=table header size.
}

void PSI_Syntax::pack(std::vector<uint8_t> &data, std::vector<uint8_t>::iterator &offset) {
	uint8_t *dataPointer=&(offset[0]);

	Utility::byteify<uint16_t>( dataPointer, extension_ );
	dataPointer += 2;

	if( version_ >= (1<<5) ) {
		throw PSI_ValueException("A version number value is set too high in a PSI syntax section.");
	}

	//Reserved bits + version_ + currentIndicator_
	uint8_t byte = (0x3 << 6) | (version_ <<1) | currentIndicator_;
	*dataPointer++ = byte;
	*dataPointer++= sectionNumber_;
	*dataPointer++= lastSectionNumber_;

	offset+=5;
}

void PSI_Syntax::unpack(const std::vector<uint8_t> &data,
			std::vector<uint8_t>::const_iterator &offset,
			int sectionLength)
{
	int overhead=5+4; //5 byte header, 4 byte crc.
	if (sectionLength < overhead) {
		throw PSI_FormatException("The length of a PSI Syntax section needs to be at least 9 bytes.");
	}

	const uint8_t *dataPointer=&data[0];

	extension_=Utility::debyteify<uint16_t>( dataPointer );
	dataPointer+=2;
	uint8_t value=*dataPointer++;

	currentIndicator_ = value & 1;
	version_ = (value >> 1) & ((1<<5)-1);

	//Check reserved, this is strict checker that validates
	//we are doing it right.
	if((value>>6) != 3) {
		throw PSI_FormatException("Reserved bits not set in table syntax section.");
	}
	sectionNumber_=*dataPointer++;
	lastSectionNumber_=*dataPointer++;

	offset+=5;
}


