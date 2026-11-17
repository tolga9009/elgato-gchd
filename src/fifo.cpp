/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include "fifo.hpp"
#include "output_io.hpp"
#include "process.hpp"
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
static bool privateDirectory(const std::string &path) {
    struct stat st{};
    return lstat(path.c_str(),&st)==0 && S_ISDIR(st.st_mode) && st.st_uid==geteuid() && (st.st_mode&0077)==0;
}
std::string Fifo::defaultPath() {
    const char *runtime=std::getenv("XDG_RUNTIME_DIR");
    if (runtime && privateDirectory(runtime)) return std::string(runtime)+"/gchd.ts";
    const std::string directory="/tmp/gchd-"+std::to_string(geteuid());
    if (mkdir(directory.c_str(),0700)!=0 && errno!=EEXIST) throw std::runtime_error("Cannot create private runtime directory");
    if (!privateDirectory(directory)) throw std::runtime_error("Unsafe runtime directory: "+directory);
    return directory+"/gchd.ts";
}
int Fifo::enable(std::string output) {
    const bool defaultOutput=output.empty();
    output_=defaultOutput ? defaultPath() : output;
    lockFd_=open((output_+".lock").c_str(),O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
    struct stat lockStat{};
    if (lockFd_<0 || fstat(lockFd_,&lockStat)!=0 || !S_ISREG(lockStat.st_mode) || lockStat.st_uid!=geteuid() || (lockStat.st_mode&0077)!=0 || flock(lockFd_,LOCK_EX|LOCK_NB)!=0) {
        std::cerr << "Cannot lock FIFO " << output_ << ": " << std::strerror(errno) << '\n'; return 1;
    }
    if (mkfifo(output_.c_str(),0600)==0) created_=true;
    else if (!(errno==EEXIST && defaultOutput)) {
        std::cerr << "Cannot create FIFO " << output_ << ": " << std::strerror(errno) << '\n'; return 1;
    }
    struct stat st{};
    if (lstat(output_.c_str(),&st)!=0 || !S_ISFIFO(st.st_mode) || st.st_uid!=geteuid() || (st.st_mode&0077)!=0) {
        created_=false; std::cerr << "Unsafe existing FIFO: " << output_ << '\n'; return 1;
    }
    device_=st.st_dev; inode_=st.st_ino;
    std::cerr << "FIFO: " << output_ << "; waiting for reader\n";
    while (Process::isActive()) {
        fd_=open(output_.c_str(),O_WRONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC);
        if (fd_>=0) {
            struct stat opened{};
            if (fstat(fd_,&opened)!=0 || opened.st_dev!=device_ || opened.st_ino!=inode_) {
                close(fd_); fd_=-1; throw std::runtime_error("FIFO replaced while opening");
            }
            return 0;
        }
        if (errno!=ENXIO && errno!=EINTR) {
            std::cerr << "Cannot open FIFO: " << std::strerror(errno) << '\n'; return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return 1;
}
void Fifo::disable() noexcept {
    if (fd_>=0) { close(fd_); fd_=-1; }
    if (created_) {
        struct stat st{};
        if (lstat(output_.c_str(),&st)==0 && S_ISFIFO(st.st_mode) && st.st_dev==device_ && st.st_ino==inode_) unlink(output_.c_str());
        created_=false;
    }
    if (lockFd_>=0) { close(lockFd_); lockFd_=-1; }
}
void Fifo::output(std::vector<unsigned char> *buffer, const std::atomic<bool> *cancel) {
    if (fd_>=0) OutputIO::writeAll(fd_,buffer->data(),buffer->size(),cancel);
}
