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

#include <sys/file.h>
#include <sys/stat.h>

#include <process.hpp>

std::atomic<bool> Process::isActive_;

bool Process::isActive() {
	return isActive_;
}

void Process::setActive(bool isActive) {
	isActive_ = isActive;
}

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
	if (pidFd_) {
		flock(pidFd_, LOCK_UN);
		close(pidFd_);
	}

	if (hasPid_) {
		unlink(pidPath_.c_str());
		hasPid_ = false;
	}
}

void Process::sigHandler_(int sig) {
	std::cerr << std::endl << "Stop signal received." << std::endl;

	switch(sig) {
		case SIGINT:
			setActive(false);
			break;
		case SIGTERM:
			setActive(false);
			break;
	}
}

Process::Process() {
	isActive_ = false;
	hasPid_ = false;
	pidFd_ = 0;

	// signal handling
	struct sigaction action;
	action.sa_handler = sigHandler_;
	sigaction(SIGINT, &action, nullptr);
	sigaction(SIGTERM, &action, nullptr);

	// ignore SIGPIPE, else program terminates on unsuccessful write()
	struct sigaction ignore;
	ignore.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ignore, nullptr);
}

Process::~Process() {
	destroyPid();
}
