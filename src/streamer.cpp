/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <array>
#include <iostream>

#include <streamer.hpp>
#include "output_queue.hpp"

void Streamer::loop(double durationSeconds) {
    OutputQueue writer([&](std::vector<unsigned char> &buffer, const std::atomic<bool> *cancel) {
        switch (output_) {
            case StreamOutput::Disk: case StreamOutput::Stdout: disk.output(&buffer,cancel); break;
            case StreamOutput::FIFO: fifo.output(&buffer,cancel); break;
            case StreamOutput::Socket: socket.output(&buffer,cancel); break;
        }
    });
    using Clock=std::chrono::steady_clock;
    const auto start=Clock::now(); auto last=start;
    uint64_t received=0, previous=0;
    const auto stats=[&](bool final) {
        const auto now=Clock::now();
        const double elapsed=std::chrono::duration<double>(now-(final ? start : last)).count();
        const uint64_t written=writer.written();
        std::cerr << (final ? "Final stats: " : "Stats: ") << written << " bytes written, " << received
                  << " USB bytes, " << (elapsed>0 ? (written-(final ? 0 : previous))*8.0/elapsed/1000000 : 0)
                  << " Mbps, " << writer.dropped() << " dropped buffers (" << writer.droppedBytes()
                  << " bytes), " << gchd_->overflows() << " USB overflows, " << gchd_->timeouts() << " timeouts\n";
        previous=written; last=now;
    };
    std::cerr << "Streamer has been started.\n";
    try {
        while (process_->isActive()) {
            const auto now=Clock::now();
            if (durationSeconds>0 && std::chrono::duration<double>(now-start).count()>=durationSeconds) break;
            writer.check();
            std::vector<unsigned char> buffer;
            gchd_->stream(&buffer);
            received+=buffer.size();
            if (!buffer.empty()) writer.push(std::move(buffer));
            if (Clock::now()-last>=std::chrono::seconds(5)) stats(false);
        }
        writer.finish();
    } catch (...) {
        try { writer.finish(); } catch (...) {}
        stats(true); throw;
    }
    stats(true);
}

Streamer::Streamer(GCHD *gchd, Process *process, StreamOutput output): output_(output), gchd_(gchd), process_(process) {}
