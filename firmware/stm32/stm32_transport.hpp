#pragma once

#include "firmware/core/interfaces/IByteTransport.hpp"

namespace rtu {

// STM32 implementation of IByteTransport over UART/RS-485.
class Stm32Transport : public IByteTransport {
public:
    bool send(const uint8_t* data, std::size_t len) override;
    std::size_t receive(uint8_t* data, std::size_t max_len) override;
};

} // namespace rtu