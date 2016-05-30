/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <csignal>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

#include <fifo.hpp>

constexpr auto POLL_TIMEOUT =	1;
constexpr auto MAX_QUEUE =	8;
constexpr auto NUM_POLL =	1;

int Fifo::enable(std::string output) {
	output_ = output;

	// ignore SIGPIPE, else program terminates on unsuccessful write()
	struct sigaction ignore;
	ignore.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ignore, nullptr);

	if (mkfifo(output_.c_str(), 0644)) {
		std::cerr << "Error creating FIFO." << std::endl;

		return 1;
	}

	std::cerr << "FIFO: " << output << " has been created." << std::endl;
	fd_ = open(output_.c_str(), O_WRONLY);

	if (fd_ < 0) {
		std::cerr << "Can't open FIFO for writing." << std::endl;
	}

	auto flags = fcntl(fd_, F_GETFL);

	if (flags == -1) {
		std::cerr << "FIFO error: fcntl get flags." << std::endl;

		return 1;
	}

	flags |= O_NONBLOCK;

	if (fcntl(fd_, F_SETFL, flags) == -1) {
		std::cerr << "FIFO error: fcntl set flags." << std::endl;

		return 1;
	}

	pollFd_.fd = fd_;
	pollFd_.events = POLLOUT;

	return 0;
}

void Fifo::disable() {
	if (fd_) {
		close(fd_);
		fd_ = -1;
		unlink(output_.c_str());
	}
}

void Fifo::output(std::array<unsigned char, DATA_BUF> *buffer) {
	if (fd_ == -1) {
		return;
	}

	if (poll(&pollFd_, NUM_POLL, POLL_TIMEOUT)
			&& pollFd_.revents == POLLOUT) {
		// POLLOUT returns, when FIFO is ready
		if (!queue_.empty()) {
			queue_.push(*buffer);

			while (poll(&pollFd_, NUM_POLL, POLL_TIMEOUT)
					&& pollFd_.revents == POLLOUT
					&& !queue_.empty()) {
				if (!handleWrite(&queue_.front())) {
					queue_.pop();
				}
			}
		} else if(handleWrite(buffer)) {
			queue_.push(*buffer);
		}
	} else if (pollFd_.revents & POLLERR) {
		// POLLERR returns, when FIFO has been closed
		if (!queue_.empty()) {
			std::queue<std::array<unsigned char, DATA_BUF>> empty;
			queue_.swap(empty);
		}
	} else {
		// reader has missed some packets, queue packets
		queue_.push(*buffer);

		if (queue_.size() > MAX_QUEUE) {
			queue_.pop();
		}
	}
}

int Fifo::handleWrite(std::array<unsigned char, DATA_BUF> *buffer) {
	long ret;
	unsigned long length = buffer->size();

	if (bytesWritten_) {
		length -= static_cast<unsigned long>(bytesWritten_);
		ret = write(fd_, &buffer->data()[bytesWritten_], length);
	} else {
		ret = write(fd_, buffer->data(), length);
	}

	// partial write has happened
	if (ret < static_cast<long>(length)) {
		// don't forget about two and more partial writes in a row
		bytesWritten_ += ret;

		return 1;
	}

	// no partial write has happened, reset counter
	bytesWritten_ = 0;

	return 0;
}

Fifo::Fifo() {
	bytesWritten_ = 0;
	fd_ = -1;
}

Fifo::~Fifo() {
	disable();
}
