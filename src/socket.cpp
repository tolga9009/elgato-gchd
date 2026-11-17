/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include "socket.hpp"
#include "output_io.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <cstring>
#include <cerrno>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
int Socket::enable(std::string ip, std::string port) {
    addrinfo hints{}; hints.ai_family=AF_UNSPEC; hints.ai_socktype=SOCK_DGRAM;
    addrinfo *raw=nullptr;
    const int status=getaddrinfo(ip.empty() ? "127.0.0.1" : ip.c_str(),port.c_str(),&hints,&raw);
    if (status) { std::cerr << "Socket address: " << gai_strerror(status) << '\n'; return 1; }
    std::unique_ptr<addrinfo,decltype(&freeaddrinfo)> addresses(raw,freeaddrinfo);
    for (addrinfo *entry=raw; entry; entry=entry->ai_next) {
        int candidate=::socket(entry->ai_family,entry->ai_socktype|SOCK_NONBLOCK|SOCK_CLOEXEC,entry->ai_protocol);
        if (candidate<0) continue;
        if (connect(candidate,entry->ai_addr,entry->ai_addrlen)==0) { fd_=candidate; break; }
        close(candidate);
    }
    if (fd_<0) { std::cerr << "Socket connect: " << std::strerror(errno) << '\n'; return 1; }
    std::cerr << "SOCKET: Connecting to " << (ip.empty() ? "127.0.0.1" : ip) << ':' << port << '\n'; return 0;
}
void Socket::disable() { if (fd_>=0) { close(fd_); fd_=-1; } }
void Socket::output(std::vector<unsigned char> *buffer, const std::atomic<bool> *cancel) {
    if (fd_<0) return;
    // Each upstream USB buffer remains one UDP datagram. Never split a short datagram.
    for (;;) {
        if (cancel && cancel->load()) throw output_cancelled();
        const ssize_t sent=send(fd_,buffer->data(),buffer->size(),MSG_NOSIGNAL);
        if (sent==static_cast<ssize_t>(buffer->size())) return;
        if (sent<0 && errno==EINTR) continue;
        if (sent<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue;
        }
        throw std::runtime_error(std::string("UDP output failed: ")+(sent<0 ? std::strerror(errno) : "short datagram"));
    }
}
Socket::Socket(): fd_(-1) {}
Socket::~Socket() { disable(); }
