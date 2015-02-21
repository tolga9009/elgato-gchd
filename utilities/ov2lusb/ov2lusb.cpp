#include <fstream>
#include <iostream>
#include <regex>
#include <string>

int main(int argc, char **argv) {
	/* exit, if number of args is incorrect */
	if(argc != 3) {
		std::cout << "Usage: " << argv[0] << " <file> <device destination>" << std::endl;

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
	std::string str_rgx_setup = "SETUP:.*([[:digit:]]).([[:digit:]])";

	std::regex rgx_setup(str_rgx_setup);
	std::smatch match;

	int line = 0;

	/* searching for matches, line for line using regex */
	while(!textfile.eof()) {
		++line;
		std::getline(textfile, strbuf);

		if(std::regex_search(strbuf, match, rgx_setup)) {
			for(unsigned long i = 0; i < match.size(); i++) {
				if(match[1] != device) {
					std::cout << "Breaking loop" << std::endl;
					break;
				} else {
					std::cout << "Line " << line << ": Match: " << match[i] << std::endl;
				}
			}
		}
	}
}
