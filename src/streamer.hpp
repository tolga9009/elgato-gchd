/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef STREAMER_CLASS_H
#define STREAMER_CLASS_H

#include <fstream>
#include <string>

#include <poll.h>

#include <fifo.hpp>
#include <gchd.hpp>
#include <process.hpp>
#include <socket.hpp>

class Streamer {
	public:
		int enableDisk(std::string diskPath);
		void disableDisk();
		void loop();
		Fifo fifo;
		Socket socket;
		Streamer(GCHD *gchd, Process *process);
		~Streamer();

	private:
		std::ofstream diskStream_;
		GCHD *gchd_;
		Process *process_;
};

#endif
