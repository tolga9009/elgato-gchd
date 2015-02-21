#include <iostream>
#include <fstream>
#include <regex>

int main() {
	int line = 0;
	std::string str;

	std::ifstream input("input.txt");

	std::string regex_str = "SETUP:.*([[:digit:]].[[:digit:]])";
	std::regex reg(regex_str);
	std::smatch match;

	while(!input.eof()) {
		++line;
		std::getline(input, str);

		if(std::regex_search(str, match, reg)) {
			std::cout << "String found @ line " << line << std::endl;

			for(unsigned long i = 0; i < match.size(); i++) {
				std::cout << "Match: " << match[i] << std::endl;
			}
		}
	}
}
