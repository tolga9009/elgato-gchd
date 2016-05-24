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

#include <gchd.hpp>
#include <process.hpp>

class Streamer {
	public:
		int enableDisk(std::string diskPath);
		void disableDisk();
		int enableFifo(std::string fifoPath);
		void disableFifo();
		int enableSocket(std::string ip, std::string port);
		void disableSocket();
		void loop();
		Streamer(GCHD *gchd, Process *process);
		~Streamer();

	private:
		bool hasFifo_;
		int fifoFd_;
		int socketFd_;
		std::ofstream diskStream_;
		std::string fifoPath_;
		GCHD *gchd_;
		Process *process_;
};

#endif
