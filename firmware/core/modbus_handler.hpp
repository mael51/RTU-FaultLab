#pragma once

#include "firmware/core/modbus_frame.hpp"

#include <cstddef>
#include <cstdint>

namespace rtu {

class RegisterMap;

// Handles a parsed request against the register array.
// Returns number of bytes written to response (0 = no response, e.g. wrong slave).
// response capacity must be >= 256.
int handleModbusRequest(const ModbusRequest& req,
                       uint16_t* regs, std::size_t reg_count,
                       uint8_t* response, std::size_t resp_capacity);

// Convenience overload taking a RegisterMap.
int handleModbusRequest(const ModbusRequest& req,
                       RegisterMap& regs,
                       uint8_t* response, std::size_t resp_capacity);

} // namespace rtu