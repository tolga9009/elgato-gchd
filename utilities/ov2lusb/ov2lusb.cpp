#include <fstream>
#include <iostream>
#include <regex>
#include <string>

int main(int argc, char **argv) {
	/* exit, if number of args is incorrect */
	if(argc != 3) {
		std::cout << "Usage: " << argv[0] << " <file> <device number>" << std::endl;

		return 1;
	}

	std::string filename(argv[1]);
	std::string device(argv[2]);

	/* open up the file */
	std::ifstream textfile(filename);

	if(!textfile) {
		std::cout << "File not found. Terminating." << std::endl;

		return 1;
	}

	/* setting up regex */
	std::string strbuf;

	/* SETUP
	 * match[1]: device number
	 * match[2]: endpoint
	 */
	std::string str_rgx_setup = "SETUP:\\s(\\d)\\.(\\d)";

	/* IN
	 * match[1]: device number
	 * match[2]: endpoint
	 */
	std::string str_rgx_in = "IN\\s*:\\s(\\d)\\.(\\d)";

	/* OUT
	 * match[1]: device number
	 * match[2]: endpoint
	 */
	std::string str_rgx_out = "OUT\\s*:\\s(\\d)\\.(\\d)";

	/* DATA0
	 * match[1]: bmRequestType
	 * match[2]: bRequest
	 * match[3-4]: wValue (bytes swapped)
	 * match[5-6]: wIndex (bytes swapped)
	 * match[7-8]: wLength (bytes swapped)
	 */
	std::string str_rgx_data0 = "DATA0:\\s(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)";
	std::string str_rgx_data1 = "DATA1:\\s(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)(?:([0-9a-f]+)*\\s?)";

	std::string str_rgx_ack = "ACK";
	std::string str_rgx_nak = "NAK";

	std::regex rgx_setup(str_rgx_setup);
	std::regex rgx_in(str_rgx_in);
	std::regex rgx_out(str_rgx_out);
	std::regex rgx_data0(str_rgx_data0);
	std::regex rgx_data1(str_rgx_data1);
	std::regex rgx_ack(str_rgx_ack);
	std::regex rgx_nak(str_rgx_nak);

	std::smatch setup_match;
	std::smatch in_match;
	std::smatch out_match;
	std::smatch data0_match;
	std::smatch data1_match;

	/* loop variables */
	int line = 0;
	std::string prev_bmRequestType;

	/* searching for matches, line for line using regex */
	while(!textfile.eof()) {
		++line;
		std::getline(textfile, strbuf);

		if(std::regex_search(strbuf, setup_match, rgx_setup)) {

			/* skip to next iteration, if device number doesn't match */
			if(setup_match[1] != device) {
				continue;
			}

			++line;
			std::getline(textfile, strbuf);

			if(std::regex_search(strbuf, data0_match, rgx_data0)) {
				std::string bmRequestType(data0_match[1]);
				std::string bRequest(data0_match[2]);
				std::string wValue(data0_match[4]);
				wValue.append(data0_match[3]);
				std::string wIndex(data0_match[6]);
				wIndex.append(data0_match[5]);
				std::string wLength(data0_match[8]);
				wLength.append(data0_match[7]);

				if(prev_bmRequestType != bmRequestType) {
					std::cout << std::endl;
				}

nextline:
				++line;
				std::getline(textfile, strbuf);

				if(std::regex_search(strbuf, rgx_ack)) {
					goto nextline;
				}

				if(std::regex_search(strbuf, in_match, rgx_in)) {
					goto nextline;
				}

				if(std::regex_search(strbuf, rgx_nak)) {
					goto nextline;
				}

				if(std::regex_search(strbuf, out_match, rgx_out)) {
					++line;
					std::getline(textfile, strbuf);

					if(std::regex_search(strbuf, data1_match, rgx_data1)) {
						goto print;
					}
				}

print:
				if(bmRequestType == "c0") {
					std::cout << "read_config" << "(0x" << bRequest << ", 0x" << wValue << ", 0x" << wIndex << ", " << std::stoi(wLength, nullptr, 16) << ");" << std::endl;
				} else if(bmRequestType == "40") {
					if(std::stoi(wLength, nullptr, 16) > 10) {
						std::cout << "Line " << line << ", wLength: " << std::stoi(wLength, nullptr, 16) << std::endl;
					}

					std::cout << "write_config" << std::stoi(wLength, nullptr, 16) << "(0x" << bRequest << ", 0x" << wValue << ", 0x" << wIndex;

					for(unsigned long i = 1; i < data1_match.size() && i <= std::stoul(wLength, nullptr, 16); i++) {
						std::cout << ", 0x" << data1_match[i];
					}

					std::cout << ");" << std::endl;
				} else {
					std::cout << "Line " << line << ": " << "(0x" << bmRequestType << ", 0x" << bRequest << ", 0x" << wValue << ", 0x" << wIndex << ", 0x" << wLength << ");" << std::endl;
				}

				prev_bmRequestType = bmRequestType;
			}
		}
	}
}
