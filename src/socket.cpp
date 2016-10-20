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

#include <socket.hpp>

int Socket::enable(std::string ip, std::string port) {
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

	//Removal of this message makes socket error cryptic sometimes.
	std::cerr << "SOCKET: Binding to address ";
	if( ipAddress != NULL ) {
		std::cerr << "[" <<ipAddress << "]";
	}
	std::cerr << ":" <<port <<std::endl;

	if (ret) {
		std::cerr << "Socket error: " << gai_strerror(ret) << std::endl;
		return 1;
	}

	for (entry = result; entry != nullptr; entry = entry->ai_next) {
		fd_ = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);

		if (fd_ < 0) {
			// error, try next iteration
			continue;
		}

		if (connect(fd_, entry->ai_addr, entry->ai_addrlen) != -1) {
			// success
			break;
		}

		close(fd_);
		fd_ = -1;

		return 1;
	}

	freeaddrinfo(result);

	auto flags = fcntl(fd_, F_GETFL);

	if (flags == -1) {
		std::cerr << "Socket error: fcntl get flags." << std::endl;

		return 1;
	}

	flags |= O_NONBLOCK;

	if (fcntl(fd_, F_SETFL, flags) == -1) {
		std::cerr << "Socket error: fcntl set flags." << std::endl;

		return 1;
	}

	return 0;
}

void Socket::disable() {
	if (fd_ != -1) {
		close(fd_);
		fd_ = -1;
	}
}

void Socket::output(std::vector<unsigned char> *buffer) {
	if (fd_ == -1) {
		return;
	}

	write(fd_, buffer->data(), buffer->size());
}

Socket::Socket() {
	fd_ = -1;
}

Socket::~Socket() {
	disable();
}
