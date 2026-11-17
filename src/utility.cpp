/**
 * Copyright (c) 2016 Scott Dossey <seveirein@yahoo.com>
 *
 * This source file is part of Game Capture HD Linux driver and is distributed
 * under the MIT License. For more information, see LICENSE file.
 */

#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <climits>
#include <iterator>
#include <cmath>
#include <sstream>
#include <algorithm>
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

	void split(const std::string &input,
		   char delimiter,
		   std::vector<std::string> &elements) {
		std::stringstream stream;
		stream.str(input);
		std::string item;
		while (std::getline(stream, item, delimiter)) {
			elements.push_back(item);
		}
	}

	// trim from start
	std::string &ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(),
						[](unsigned char c) { return !std::isspace(c); }));
		return s;
	}

	// trim from end
	std::string &rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(),
				     [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
		return s;
	}

	// trim from both ends
	std::string &trim(std::string &s) {
		return ltrim(rtrim(s));
	}

	std::vector<std::string> split(const std::string &input, char delimiter) {
		std::vector<std::string> elements;
		split(input, delimiter, elements);
		return elements;
	}

	//This works with IPV4 and IPV6 address to get port and address
	//IE:
	//   ::1 is an IPV6 loopback
	//    :1 is port 1
	// [::1]:1 is IPV6 loopback address port 1.
	// 127.0.0.1:1 IPV4 example
	// [127.0.0.1]:1 optional IPV4 usage.
    bool parseDuration(double &seconds, const std::string &input) {
        if (input.empty()) return false;
        std::string number=input; double multiplier=1;
        if (number.back()=='s' || number.back()=='m' || number.back()=='h') {
            const char suffix=number.back(); number.pop_back();
            multiplier=suffix=='h' ? 3600 : (suffix=='m' ? 60 : 1);
        }
        char *end=nullptr; errno=0;
        const double value=std::strtod(number.c_str(),&end);
        if (end==number.c_str() || *end || errno==ERANGE || !std::isfinite(value) || value<=0 || value*multiplier>31536000) return false;
        seconds=value*multiplier; return true;
    }

	bool parseNumericResolution(unsigned long &x, unsigned long &y, const std::string &input) {
        const auto separator = input.find('x');
        if (separator == std::string::npos || input.find('x', separator+1) != std::string::npos) return false;
        unsigned long values[2]{};
        std::string parts[] = {input.substr(0, separator), input.substr(separator+1)};
        for (unsigned i=0; i<2; ++i) {
            trim(parts[i]);
            if (parts[i].empty() || !std::all_of(parts[i].begin(), parts[i].end(),
                [](unsigned char c) { return c >= '0' && c <= '9'; })) return false;
            char *end = nullptr;
            errno = 0;
            values[i] = std::strtoul(parts[i].c_str(), &end, 10);
            if (errno == ERANGE || *end || values[i] == 0 || values[i] > UINT_MAX) return false;
        }
        x=values[0]; y=values[1]; return true;
    }

    bool splitIPAddressAndPort(std::string &address, std::string &port, const std::string &passedInput) {
        address.clear(); port.clear();
        std::string input=passedInput;
        trim(input);
        if (input.empty()) return true;
        if (input.front() == '[') {
            const auto end=input.find(']');
            if (end == std::string::npos) return false;
            address=input.substr(1, end-1); trim(address);
            std::string rest=input.substr(end+1); trim(rest);
            if (address.empty()) return false;
            if (rest.empty()) return true;
            if (rest.front() != ':') return false;
            port=rest.substr(1); trim(port);
            return port.find(':') == std::string::npos;
        }
        const auto separator=input.find(':');
        if (separator != std::string::npos && input.find(':', separator+1) == std::string::npos) {
            address=input.substr(0, separator); port=input.substr(separator+1);
            trim(address); trim(port);
        } else address=input;
        return true;
    }
}
