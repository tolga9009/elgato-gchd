#include "output_io.hpp"
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <string>
void OutputIO::writeAll(int fd, const unsigned char *data, size_t size, const std::atomic<bool> *cancel) {
    size_t offset=0;
    while (offset<size) {
        if (cancel && cancel->load()) throw output_cancelled();
        const ssize_t count=write(fd,data+offset,size-offset);
        if (count>0) { offset+=static_cast<size_t>(count); continue; }
        if (count<0 && errno==EINTR) continue;
        if (count<0 && (errno==EAGAIN || errno==EWOULDBLOCK)) {
            pollfd ready{fd,POLLOUT,0};
            const int status=poll(&ready,1,100);
            if (status>=0 || errno==EINTR) continue;
        }
        if (count<0 && errno==EPIPE) throw std::runtime_error("Output reader disconnected (EPIPE)");
        throw std::runtime_error(std::string("Output write failed: ") + (count==0 ? "zero-length write" : std::strerror(errno)));
    }
}
