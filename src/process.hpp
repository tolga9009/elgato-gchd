/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef PROCESS_CLASS_H
#define PROCESS_CLASS_H

#include <string>

class Process {
	public:
		int createPid(std::string pidPath);
		void destroyPid();
		int createFifo(std::string fifoPath);
		void destroyFifo();
                Process();
		~Process();

	private:
		bool hasPid_;
		bool hasFifo_;
		int pidFd_;
		int fifoFd_;
		std::string pidPath_;
		std::string fifoPath_;
};

#endif
