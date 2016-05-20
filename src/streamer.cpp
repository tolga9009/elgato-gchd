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

	hasFifo_ = true;
	std::cerr << fifoPath << " has been created." << std::endl;
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

	std::cerr << "Streaming data from device now." << std::endl;

	while (process_->isActive() && fifoFd_) {
		std::vector<unsigned char> buffer(DATA_BUF);
		std::unique_lock<std::mutex> lock(*gchd->getMutex());

		while(gchd->getQueue()->empty()) {
			gchd->getCv()->wait(lock);
		}

		buffer = gchd->getQueue()->front();
		gchd->getQueue()->pop();
		lock.unlock();
		write(fifoFd_, reinterpret_cast<char *>(buffer.data()), DATA_BUF);
	}
}

void Streamer::streamToDisk(GCHD *gchd, std::string outputPath) {
	std::ofstream output(outputPath, std::ofstream::binary);
	std::cerr << "Streaming data from device now." << std::endl;

	while (process_->isActive()) {
		std::vector<unsigned char> buffer(DATA_BUF);
		std::unique_lock<std::mutex> lock(*gchd->getMutex());

		while(gchd->getQueue()->empty()) {
			gchd->getCv()->wait(lock);
		}

		buffer = gchd->getQueue()->front();
		gchd->getQueue()->pop();
		lock.unlock();

		// TODO reinterpret really needed?
		output.write(reinterpret_cast<char *>(buffer.data()), DATA_BUF);
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
