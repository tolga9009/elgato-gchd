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

#ifndef PSI_DATA_H
#define PSI_DATA_H

#include <vector>
#include <cstdint>

class PSI_Data
{
	public:
		//Overload the following 3 methods.
		virtual int calculateSize()=0; //Abstract

		virtual void pack(std::vector<uint8_t> &outputData,
				  std::vector<uint8_t>::iterator &offset)=0; //Abstract

		virtual void unpack(const std::vector<uint8_t> &inputData,
				    std::vector<uint8_t>::const_iterator &offset,
				    int size )=0; //Abstract

		//Do not overload this.
		virtual void bytes( std::vector<uint8_t> &outputData );
};

#endif


