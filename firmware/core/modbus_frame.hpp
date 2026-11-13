#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rtu {

// Fixed-size frame buffer (NFR-002: bounded, max 256 bytes).
struct ModbusFrame {
    std::array<uint8_t, 256> data{};
    std::size_t len = 0;
};

struct ModbusRequest {
    uint8_t  slave    = 0;
    uint8_t  function = 0;
    uint16_t address  = 0;
    uint16_t value    = 0; // FC06
    uint16_t count    = 0; // FC03
    bool     crc_ok   = false;
};

} // namespace rtu