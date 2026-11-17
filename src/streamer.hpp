/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#ifndef STREAMER_CLASS_H
#define STREAMER_CLASS_H

#include <disk.hpp>
#include <fifo.hpp>
#include <gchd.hpp>
#include <process.hpp>
#include <socket.hpp>

enum class StreamOutput { Disk, FIFO, Socket, Stdout };

class Streamer {
	public:
		void loop(double durationSeconds=0);
		Disk disk;
		Fifo fifo;
		Socket socket;
		Streamer(GCHD *gchd, Process *process, StreamOutput output=StreamOutput::FIFO);

	private:
		StreamOutput output_;
		GCHD *gchd_;
		Process *process_;
};

#endif
