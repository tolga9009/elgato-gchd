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
						std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	// trim from end
	std::string &rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(),
				     std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
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
	bool splitIPAddressAndPort( std::string &address, std::string &port, const std::string &passedInput) {
		address="";
		port="";

		std::string input=passedInput;
		input=trim(input); //Trim modifies original;
		//Check for wrapped "[address]"
		int ipBegin=0;
		int ipEnd=0;
		std::string remainder=input;
		if( input[0] == '[') {
			++ipBegin;
			++ipEnd;
			//Scan for end
			while( true ) {
				if (input[ipEnd]==0) { //Didn't find end, pass whole weird thing as
					//address, some sort of future address.
					break;
				}
				if (input[ipEnd]==']') {
					std::string subString=input.substr( ipBegin, ipEnd-ipBegin );
					address=trim(subString); //trim modifies subString
					//return substr.
					remainder=input.substr(ipEnd + 1);
					remainder=trim(remainder); //trim modifies original
					break;
				}
				++ipEnd;
			}
		}   //Okay that takes care of "[address]"

		std::vector<std::string> parts=split( remainder, ':' );

		if (parts.size()==2) { // :port, ipv4:port, [ipv6]:port, or [ipv6]junk:port
			//ipv6 address is is not, because they have at least two ":" characters.
			std::string potentialAddress=trim(parts[0]); //This modifies parts[0]
			if(potentialAddress.size() != 0 ) {
				if( address.size() == 0 ) {
					address=potentialAddress;
				} else {
					//Something like [::1]3:1, gotta treat as invalid
					return false;;
				}
			}
			port=trim(parts[1]); //This modifies parts[1]
		} else {
			//Gotta be an ipv6 address by itself.
			if( address.size() == 0 ) {
				address=input;
			} else if (remainder.size()!=0) {
				//Something like [::1]junk
				return false;
			}
		}
		return true;
	}

}

