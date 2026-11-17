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

// constants
constexpr auto version = "0.3.0";

std::atomic<bool> Process::isActive_;
volatile sig_atomic_t Process::stopSignal_ = 0;

bool Process::isActive() {
	return isActive_ && stopSignal_ == 0;
}

void Process::setActive(bool isActive) {
	isActive_ = isActive;
}

std::string Process::getName() {
	if (name_.empty()) {
		name_ = "gchd";
	}

	return name_;
}

void Process::setName(std::string name) {
	name_ = name;
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
		pidFd_ = -1;

		return -1;
	}

	hasPid_ = true;

	return 0;
}

void Process::destroyPid() {
	if (pidFd_ >= 0) {
		flock(pidFd_, LOCK_UN);
		close(pidFd_);
		pidFd_ = -1;
	}

	if (hasPid_) {
		unlink(pidPath_.c_str());
		hasPid_ = false;
	}
}

std::string Process::getVersion() {
	return version;
}

void Process::sigHandler(int sig) {
    if (stopSignal_) _exit(128 + sig);
    stopSignal_ = sig;
}

Process::Process() {
	isActive_ = false;
	hasPid_ = false;
	pidFd_ = -1;
	stopSignal_ = 0;

	// signal handling
	struct sigaction action{};
	//{} is critical because it value initializes it to zero.

	action.sa_handler = sigHandler;
	sigfillset(&action.sa_mask);
	sigaction(SIGINT, &action, nullptr);
	sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGHUP, &action, nullptr);
    struct sigaction ignore{};
    ignore.sa_handler=SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    sigaction(SIGPIPE,&ignore,nullptr);
}

Process::~Process() {
	destroyPid();
}
