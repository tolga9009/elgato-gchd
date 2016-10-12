/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <cstdint>
#include <climits>
#include <iterator>

namespace Utility
{

    template<typename T, typename I>
    static T debyteify(I iterator, unsigned limit=UINT_MAX) 
    {
        T value;
        //Cut off end bytes if limit is too small.
        unsigned bytes=std::min(sizeof(T), (size_t)limit);
        value=0;
        for(unsigned i=0; i<bytes; ++i)
        {
            value <<= 8;
            value |= *iterator++;
        }
        return value;
    };


    template<typename T, typename I>
    static void byteify(I iterator, T value, unsigned limit=UINT_MAX)
    {
        //Cut off high bits if limit is too small.
        unsigned bytes=std::min(sizeof(T), (size_t)limit);
        while(bytes > 0)
        {
            --bytes;
            *iterator++ = (value >> (bytes * 8)) & 0xff;
        }
    };

    typedef struct { 
        int32_t num;
        uint32_t denom;
    } fraction_t;

    //Finds a fractional approximation, iven a float, and a max denominator.
    //MAKE sure the the maximum possible value * maxDenom < INT32_MAX
    //before using.
    fraction_t findFraction( double value, uint32_t maxDenom );
 
};

//Some commands take a std::vector<unsigned char>
//
//With this macro you can pass a literal array definition 
//into such a function, without having to type out all the boilerplate.
//IE:
//   foo( VC{ 1, 2, 'a'} );
//This will only compile on c++11 and up.
#define VC std::vector<unsigned char> 

#endif
