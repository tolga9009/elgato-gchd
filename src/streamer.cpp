/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <array>
#include <iostream>

#include <streamer.hpp>

void Streamer::loop() {
	std::vector<unsigned char> buffer=std::vector<unsigned char>(DATA_BUF);

    if (process_->isActive()) {
	    std::cerr << "Streamer has been started." << std::endl;
    }
    while (process_->isActive()) {
        gchd_->stream(&buffer);
        if( buffer.size() != 0 ) {
            disk.output(&buffer);
            fifo.output(&buffer);
            socket.output(&buffer);
        }
	}
}

Streamer::Streamer(GCHD *gchd, Process *process) {
	gchd_ = gchd;
	process_ = process;
}
