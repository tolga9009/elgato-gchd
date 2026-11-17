#pragma once
#include <atomic>
#include <cstddef>
#include <stdexcept>
class output_cancelled: public std::runtime_error {
public: output_cancelled(): std::runtime_error("Output cancelled") {}
};
namespace OutputIO {
void writeAll(int fd, const unsigned char *data, size_t size, const std::atomic<bool> *cancel=nullptr);
}
