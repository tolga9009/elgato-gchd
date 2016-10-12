/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

/* This file holds exception used for the parsing and creation of 
 * of program specific information as specified by ISO/IEC 13818-1
 */
#ifndef PSI_EXCEPTIONS_H
#define PSI_EXCEPTIONS_H

#include <stdexcept>

class PSI_FormatException : public std::logic_error {
    public:
        using std::logic_error::logic_error; //inherit constructors
};

class PSI_ValueException : public std::logic_error {
    public:
        using std::logic_error::logic_error; //inherit constructors

};

#endif


