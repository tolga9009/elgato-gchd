/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef PROCESS_CLASS_H
#define PROCESS_CLASS_H

#include <atomic>
#include <string>

#include <core/gchd.hpp>

class Process {
	public:
		static bool isActive();
		static void setActive(bool isActive);
		int createPid(std::string pidPath);
		void destroyPid();
		int createFifo(std::string fifoPath);
		void destroyFifo();
		void streamToFifo(GCHD *gchd);
		void streamToDisk(GCHD *gchd, std::string outputPath);
		Process();
		~Process();

	private:
		static std::atomic<bool> isActive_;
		bool hasPid_;
		bool hasFifo_;
		int pidFd_;
		int fifoFd_;
		std::string pidPath_;
		std::string fifoPath_;
		static void sigHandler_(int sig);
};

#endif
