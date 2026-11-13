#pragma once

#include "firmware/core/interfaces/IByteTransport.hpp"

#include <cstdint>
#include <string>

namespace rtu {

// Serial transport (UART/RS-485) for host PC.
// Uses native APIs (Windows: CreateFile; Linux: termios).
class SerialTransport : public IByteTransport {
public:
    explicit SerialTransport(const std::string& port, uint32_t baud = 19200);
    ~SerialTransport() override;

    bool open();
    void close();

    bool send(const uint8_t* data, std::size_t len) override;
    std::size_t receive(uint8_t* data, std::size_t max_len) override;

    bool isOpen() const { return handle_ != nullptr; }

private:
    std::string port_;
    uint32_t baud_;
    void* handle_; // HANDLE on Windows, int fd on Linux
};

} // namespace rtu