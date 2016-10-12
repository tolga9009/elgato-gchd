/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <cstdint>
#include <climits>
#include <iterator>
#include <cmath>
#include "utility.hpp"

namespace Utility
{
    fraction_t findFraction( double value, uint32_t maxDenom )
    {
        //Uses Farey sequence method of approximation.
        fraction_t minF= { (int32_t) std::floor( value ), 1 };
        fraction_t maxF= { (int32_t) std::ceil( value ), 1 };
    
        while( std::max(minF.denom, maxF.denom) <= maxDenom ) {
            fraction_t mediantF = { minF.num+maxF.num, minF.denom + maxF.denom };
            double mediant= ((double)mediantF.num)/mediantF.denom;
            
            if ( value == mediant ) {
                if (mediantF.denom <= maxDenom)  {
                    return mediantF;
                }
                else if (maxF.denom > minF.denom ) {
                    return maxF;
                } else {
                    return minF;
                }
            } else if ( value > mediant ) {
                minF=mediantF;
            } else {
                maxF=mediantF;
            }
        }
        if ( minF.denom <= maxDenom ) {
            return minF;
        } else {
            return maxF;
        }
    }
}
        
