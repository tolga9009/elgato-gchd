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
#include <sys/stat.h>

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

int Streamer::enableFifo(std::string fifoPath) {
	fifoPath_ = fifoPath;

	if (mkfifo(fifoPath_.c_str(), 0644)) {
		std::cerr << "Error creating FIFO." << std::endl;

		return 1;
	}

	hasFifo_ = true;
	std::cerr << "FIFO: " << fifoPath << " has been created." << std::endl;
	fifoFd_ = open(fifoPath_.c_str(), O_WRONLY);

	if (fifoFd_ < 0) {
		std::cerr << "Can't open FIFO for writing." << std::endl;
	}

	return 0;
}

void Streamer::disableFifo() {
	if (fifoFd_) {
		close(fifoFd_);
		fifoFd_ = -1;
	}

	if (hasFifo_) {
		unlink(fifoPath_.c_str());
		hasFifo_ = false;
	}
}

int Streamer::enableSocket(std::string ip, std::string port) {
	struct addrinfo hints = {};
	struct addrinfo *result, *entry;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	// if <ip> is unconfigured, set it to nullptr, effectively IP 0.0.0.0
	const char *ipAddress;

	if (ip.empty()) {
		ipAddress = nullptr;
	} else {
		ipAddress = ip.c_str();
	}

	auto ret = getaddrinfo(ipAddress, port.c_str(), &hints, &result);

	if (ret) {
		std::cerr << "Socket error: " << gai_strerror(ret) << std::endl;

		return 1;
	}

	for (entry = result; entry != nullptr; entry = entry->ai_next) {
		socketFd_ = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);

		if (socketFd_ < 0) {
			// error, try next iteration
			continue;
		}

		if (connect(socketFd_, entry->ai_addr, entry->ai_addrlen) != -1) {
			// success
			break;
		}

		close(socketFd_);
		socketFd_ = -1;

		return 1;
	}

	freeaddrinfo(result);

	return 0;
}

void Streamer::disableSocket() {
	if (socketFd_ != -1) {
		close(socketFd_);
		socketFd_ = -1;
	}
}

void Streamer::loop() {
	std::cerr << "Streamer has been started." << std::endl;

	while (process_->isActive()) {
		std::queue<std::array<unsigned char, DATA_BUF>> queue;
		std::unique_lock<std::mutex> lock(*gchd_->getMutex());

		while(process_->isActive() && gchd_->getQueue()->empty()) {
			gchd_->getCv()->wait(lock);
		}

		gchd_->getQueue()->swap(queue);
		lock.unlock();

		while(process_->isActive() && !queue.empty()) {
			if (diskStream_.is_open()) {
				diskStream_.write(reinterpret_cast<char *>(queue.front().data()), DATA_BUF);
			}

			if (hasFifo_ && fifoFd_ != -1) {
				write(fifoFd_, queue.front().data(), DATA_BUF);
			}

			if (socketFd_ != -1) {
				write(socketFd_, queue.front().data(), DATA_BUF);
			}

			queue.pop();
		}
	}
}

Streamer::Streamer(GCHD *gchd, Process *process) {
	hasFifo_ = false;
	fifoFd_ = -1;
	socketFd_ = -1;
	gchd_ = gchd;
	process_ = process;
}

Streamer::~Streamer() {
	disableDisk();
	disableFifo();
	disableSocket();
}
