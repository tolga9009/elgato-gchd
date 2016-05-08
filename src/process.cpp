/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "process.hpp"

int Process::createPid(std::string pidPath) {
	pidPath_ = pidPath;
	pidFd_ = open(pidPath_.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (pidFd_ < 0) {
		std::cerr << "PID file could not be created." << std::endl;
		return -1;
	}

	if (flock(pidFd_, LOCK_EX | LOCK_NB) < 0) {
		std::cerr << "Could not lock PID file, another instance is already running." << std::endl;
		close(pidFd_);
		return -1;
	}

	hasPid_ = true;

	return pidFd_;
}

void Process::destroyPid() {
	flock(pidFd_, LOCK_UN);
	close(pidFd_);
	unlink(pidPath_.c_str());
	hasPid_ = false;
}

int Process::createFifo(std::string fifoPath) {
	fifoPath_ = fifoPath;

	if (mkfifo(fifoPath_.c_str(), 0644)) {
		std::cerr << "Error creating FIFO." << std::endl;
		return 1;
	}

	std::cerr << fifoPath << " has been created." << std::endl << "Waiting for user to open it." << std::endl;
	fifoFd_ = open(fifoPath_.c_str(), O_WRONLY);
	hasFifo_ = true;

	return 0;
}

void Process::destroyFifo() {
	unlink(fifoPath_.c_str());
	close(fifoFd_);
	hasFifo_ = false;
}

Process::Process() {
	hasPid_ = false;
	hasFifo_ = false;
}

Process::~Process() {
	if (hasFifo_) {
		destroyFifo();
	}

	if (hasPid_) {
		destroyPid();
	}
}
