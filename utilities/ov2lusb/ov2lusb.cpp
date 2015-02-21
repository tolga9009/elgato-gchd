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

	/* SETUP - match[1]: device destination, match[2]: endpoint */
	std::string str_rgx_setup = "SETUP:\\s(\\d)\\.(\\d)";
	std::string str_rgx_data0 = "DATA0:\\s(?:([0-9a-f]+)*\\s?)*";

	std::regex rgx_setup(str_rgx_setup);
	std::regex rgx_data0(str_rgx_data0);
	std::smatch setup_match;
	std::smatch data0_match;

	/* searching for matches, line for line using regex */
	while(!textfile.eof()) {
		std::getline(textfile, strbuf);

		if(std::regex_search(strbuf, setup_match, rgx_setup)) {

			/* skip to next iteration, if wrong device number */
			if(setup_match[1] != device) {
				std::cout << "breaking" << std::endl;
				continue;
			}

			std::getline(textfile, strbuf);

			if(std::regex_search(strbuf, data0_match, rgx_data0)) {
				for(unsigned long i = 0; i < data0_match.size(); i++) {
					std::cout << data0_match[i] << std::endl;
				}
			}
		}
	}
}
