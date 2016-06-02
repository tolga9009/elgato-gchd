/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/socket.h>

#include <streamer.hpp>

int Streamer::enableDisk(std::string diskPath) {
	diskStream_.open(diskPath, std::ofstream::binary);

	if (diskStream_.fail()) {
		std::cerr << "Can't open " << diskPath << std::endl;

		return 1;
	}

	std::cerr << "Saving to disk: " << diskPath << std::endl;

	return 0;
}

void Streamer::disableDisk() {
	if (diskStream_.is_open()) {
		diskStream_.close();
	}
}

void Streamer::loop() {
	std::array<unsigned char, DATA_BUF> buffer;

	std::cerr << "Streamer has been started." << std::endl;

	while (process_->isActive()) {
		gchd_->stream(&buffer);

		if (diskStream_.is_open()) {
			diskStream_.write(reinterpret_cast<char *>(buffer.data()), buffer.size());
		}

		fifo.output(&buffer);
		socket.output(&buffer);
	}
}

Streamer::Streamer(GCHD *gchd, Process *process) {
	gchd_ = gchd;
	process_ = process;
}

Streamer::~Streamer() {
	disableDisk();
}
