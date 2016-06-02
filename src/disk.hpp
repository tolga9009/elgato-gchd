/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef DISK_CLASS_H
#define DISK_CLASS_H

#include <array>
#include <fstream>
#include <string>

#include <gchd.hpp>

class Disk {
	public:
		int enable(std::string diskPath);
		void disable();
		void output(std::array<unsigned char, DATA_BUF> *buffer);
		Disk();
		~Disk();

	private:
		int fd_;
		std::ofstream disk_;
};

#endif
