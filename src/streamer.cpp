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

	auto flags = fcntl(socketFd_, F_GETFL);

	if (flags == -1) {
		std::cerr << "Socket error: fcntl get flags." << std::endl;

		return 1;
	}

	flags |= O_NONBLOCK;

	if (fcntl(socketFd_, F_SETFL, flags) == -1) {
		std::cerr << "Socket error: fcntl set flags." << std::endl;

		return 1;
	}

	return 0;
}

void Streamer::disableSocket() {
	if (socketFd_ != -1) {
		close(socketFd_);
		socketFd_ = -1;
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

		if (socketFd_ != -1) {
			write(socketFd_, buffer.data(), buffer.size());
		}
	}
}

Streamer::Streamer(GCHD *gchd, Process *process) {
	socketFd_ = -1;
	gchd_ = gchd;
	process_ = process;
}

Streamer::~Streamer() {
	disableDisk();
	disableSocket();
}
