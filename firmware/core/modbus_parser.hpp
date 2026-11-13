#pragma once

#include "firmware/core/modbus_frame.hpp"

#include <cstddef>
#include <cstdint>

namespace rtu {

// Returns true if the frame is structurally valid and parsed into req.
// expected_slave: the slave address this DUT responds to.
bool parseModbusRequest(const uint8_t* frame, std::size_t len,
                        uint8_t expected_slave, ModbusRequest& req);

} // namespace rtu