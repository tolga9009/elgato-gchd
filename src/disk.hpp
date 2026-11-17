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
class Disk {
    int fd_=-1;
    int restoreFlags_=-1;
public:
    int enable(const std::string &path);
    int enableStdout();
    void disable() noexcept;
    void output(std::vector<unsigned char> *buffer, const std::atomic<bool> *cancel=nullptr);
    ~Disk() { disable(); }
};
