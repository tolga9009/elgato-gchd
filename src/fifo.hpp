/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef FIFO_CLASS_H
#define FIFO_CLASS_H

#include <array>
#include <string>

#include "gchd.hpp"

class Fifo {
	public:
		int enable(std::string output);
		void disable();
		void output(std::vector<unsigned char> *buffer);
		Fifo();
		~Fifo();

	private:
        void setOpen(bool open);
        void setPaused(bool paused);
		int fd_;
		std::string output_;

        bool paused_;
        bool open_;
        bool neverOpened_;
};

#endif
