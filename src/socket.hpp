/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef SOCKET_CLASS_H
#define SOCKET_CLASS_H

#include <array>
#include <string>

#include <gchd.hpp>

class Socket {
	public:
		int enable(std::string ip, std::string port);
		void disable();
		void output(std::array<unsigned char, DATA_BUF> *buffer);
		Socket();
		~Socket();

	private:
		int fd_;
};

#endif
