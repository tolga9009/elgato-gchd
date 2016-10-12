/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* Abstract class used for reading and
 * writing data in program specific information (PSI) as specified by
 * ISO/IEC 13818-1
 *
 * This file determines an interface by which you pack/unpack psi structures.
 */
#include "psi_data.hpp"

void PSI_Data::bytes( std::vector<uint8_t> &outputVector )
{
	int size=this->calculateSize();
	outputVector.resize(size);
	std::vector<uint8_t>::iterator offset= outputVector.begin();
	this->pack( outputVector, offset );
};



