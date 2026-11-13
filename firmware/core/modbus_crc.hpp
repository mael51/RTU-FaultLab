#pragma once

#include <cstddef>
#include <cstdint>

namespace rtu {

// CRC-16/Modbus (poly 0xA001, init 0xFFFF)
uint16_t modbusCrc16(const uint8_t* data, std::size_t len);

} // namespace rtu