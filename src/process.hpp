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

class Process {
	public:
		static bool isActive();
		static void setActive(bool isActive);
		std::string getName();
		void setName(std::string name);
		int createPid(std::string pidPath);
		void destroyPid();
		Process();
		~Process();

	private:
		static std::atomic<bool> isActive_;
		bool hasPid_;
		int pidFd_;
		std::string name_;
		std::string pidPath_;
		static void sigHandler_(int sig);
};

#endif
