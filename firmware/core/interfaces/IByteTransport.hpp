#pragma once

#include <cstddef>
#include <cstdint>

namespace rtu {

class IByteTransport {
public:
    virtual ~IByteTransport() = default;
    virtual bool send(const uint8_t* data, std::size_t len) = 0;
    virtual std::size_t receive(uint8_t* data, std::size_t max_len) = 0;
};

} // namespace rtu