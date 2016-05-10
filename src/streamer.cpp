/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <fstream>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

#include <streamer.hpp>

int Streamer::createFifo(std::string fifoPath) {
	fifoPath_ = fifoPath;

	if (mkfifo(fifoPath_.c_str(), 0644)) {
		std::cerr << "Error creating FIFO." << std::endl;
		return 1;
	}

	// TODO unblock, when signal has been received
	hasFifo_ = true;
	std::cerr << fifoPath << " has been created. Waiting for user to open it." << std::endl;
	fifoFd_ = open(fifoPath_.c_str(), O_WRONLY);

	return 0;
}

void Streamer::destroyFifo() {
	if (fifoFd_) {
		close(fifoFd_);
	}

	if (hasFifo_) {
		unlink(fifoPath_.c_str());
		hasFifo_ = false;
	}
}

void Streamer::streamToFifo(GCHD *gchd) {
	// immediately return, if FIFO hasn't been opened yet
	if (!fifoFd_) {
		return;
	}

	process_->setActive(true);
	std::cerr << "Streaming data from device now." << std::endl;

	// receive audio and video from device
	while (process_->isActive() && fifoFd_) {
		unsigned char data[DATA_BUF] = {0};

		gchd->stream(data, DATA_BUF);
		write(fifoFd_, (char *)data, DATA_BUF);
	}
}

void Streamer::streamToDisk(GCHD *gchd, std::string outputPath) {
	std::ofstream output(outputPath, std::ofstream::binary);

	process_->setActive(true);
	std::cerr << "Streaming data from device now." << std::endl;

	// receive audio and video from device
	while (process_->isActive() && fifoFd_) {
		unsigned char data[DATA_BUF] = {0};

		gchd->stream(data, DATA_BUF);
		output.write((char *)data, DATA_BUF);
	}

	output.close();
}

Streamer::Streamer(Process *process) {
	hasFifo_ = false;
	fifoFd_ = 0;
	process_ = process;
}

Streamer::~Streamer() {
	destroyFifo();
}
