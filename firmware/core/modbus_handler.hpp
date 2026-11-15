#pragma once

#include <cstddef>
#include <cstdint>

#include "firmware/core/modbus_frame.hpp"

namespace rtu {

class RegisterMap;

// Handles a parsed request against the register map.
// Returns number of bytes written to response (0 = no response, e.g. wrong slave).
// response capacity must be >= 256.
int handleModbusRequest(const ModbusRequest& req, RegisterMap& regs, uint8_t* response,
                        std::size_t resp_capacity);

}  // namespace rtu