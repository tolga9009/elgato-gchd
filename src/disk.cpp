/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include "disk.hpp"
#include "output_io.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <stdexcept>
int Disk::enable(const std::string &path) {
    fd_=open(path.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
    if (fd_<0) { std::cerr << "Cannot open " << path << ": " << std::strerror(errno) << '\n'; return 1; }
    std::cerr << "Saving to disk: " << path << '\n'; return 0;
}
int Disk::enableStdout() {
    if (isatty(STDOUT_FILENO)) { std::cerr << "Refusing to write transport stream to a terminal; pipe or redirect stdout.\n"; return 1; }
    fd_=fcntl(STDOUT_FILENO,F_DUPFD_CLOEXEC,3);
    if (fd_<0) return 1;
    struct stat st{};
    if (fstat(fd_,&st)!=0) return 1;
    if (S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode)) {
        restoreFlags_=fcntl(fd_,F_GETFL);
        if (restoreFlags_<0 || fcntl(fd_,F_SETFL,restoreFlags_|O_NONBLOCK)<0) return 1;
    }
    return 0;
}
int Disk::closeOutput() noexcept {
    const int fd=fd_, restoreFlags=restoreFlags_;
    // Linux releases the descriptor even when close reports an error. Detach
    // it before closing so neither an error nor destruction can close it again.
    fd_=-1;
    restoreFlags_=-1;
    if (fd<0) return 0;
    if (restoreFlags>=0) fcntl(fd,F_SETFL,restoreFlags);
    if (close(fd)<0) return errno;
    return 0;
}
void Disk::finalize() {
    const int error=closeOutput();
    if (error) throw std::runtime_error(std::string("Output close failed: ")+std::strerror(error));
}
void Disk::disable() noexcept {
    closeOutput();
}
void Disk::output(std::vector<unsigned char> *buffer, const std::atomic<bool> *cancel) {
    if (fd_>=0) OutputIO::writeAll(fd_,buffer->data(),buffer->size(),cancel);
}
