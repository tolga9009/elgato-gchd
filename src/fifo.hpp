/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#pragma once
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
class Fifo {
    int fd_=-1, lockFd_=-1;
    bool created_=false;
    dev_t device_=0;
    ino_t inode_=0;
    std::string output_;
public:
    static std::string defaultPath();
    int enable(std::string output);
    void disable() noexcept;
    void output(std::vector<unsigned char> *buffer, const std::atomic<bool> *cancel=nullptr);
    ~Fifo() { disable(); }
};
