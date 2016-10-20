/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>

#include <disk.hpp>

int Disk::enable(std::string output) {
	disk_.open(output, std::ofstream::binary);

	if (disk_.fail()) {
		std::cerr << "Can't open " << output << std::endl;

		return 1;
	}

	std::cerr << "Saving to disk: " << output << std::endl;

	return 0;
}

void Disk::disable() {
	if (disk_.is_open()) {
		disk_.close();
	}
}

void Disk::output(std::vector<unsigned char> *buffer) {
	if (!disk_.is_open()) {
		return;
	}

	disk_.write(reinterpret_cast<char *>(buffer->data()), static_cast<long>(buffer->size()));
}

Disk::Disk() {
	fd_ = -1;
}

Disk::~Disk() {
	disable();
}
