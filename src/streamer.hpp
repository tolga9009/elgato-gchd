/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef STREAMER_CLASS_H
#define STREAMER_CLASS_H

#include <string>

#include <core/gchd.hpp>
#include <process.hpp>

class Streamer {
	public:
		int createFifo(std::string fifoPath);
		void destroyFifo();
		void streamToFifo(GCHD *gchd);
		void streamToDisk(GCHD *gchd, std::string outputPath);
		Streamer(Process *process);
		~Streamer();

	private:
		bool hasFifo_;
		int fifoFd_;
		std::string fifoPath_;
		Process *process_;
};

#endif
