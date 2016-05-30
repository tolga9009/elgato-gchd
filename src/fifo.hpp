/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef FIFO_CLASS_H
#define FIFO_CLASS_H

#include <array>
#include <queue>
#include <string>

#include <poll.h>

#include <gchd.hpp>

class Fifo {
	public:
		int enable(std::string output);
		void disable();
		void output(std::array<unsigned char, DATA_BUF> *buffer);
		Fifo();
		~Fifo();

	private:
		int fd_;
		long bytesWritten_;
		std::string output_;
		std::queue<std::array<unsigned char, DATA_BUF>> queue_;
		struct pollfd pollFd_;
		int handleWrite(std::array<unsigned char, DATA_BUF> *buffer);
};

#endif
