#include "firmware/core/modbus_parser.hpp"
#include "firmware/core/modbus_crc.hpp"
#include "firmware/core/registers.hpp"

#include <cstddef>
#include <cstdint>

namespace rtu {

bool parseModbusRequest(const uint8_t* frame, std::size_t len,
                        uint8_t expected_slave, ModbusRequest& req) {
    if (frame == nullptr || len < 8 || len > cfg::MAX_FRAME_BYTES) {
        return false;
    }

    req.slave = frame[0];
    if (req.slave != expected_slave) {
        return false; // not addressed to this DUT
    }

    req.function = frame[1];

    // CRC check (last 2 bytes, little-endian)
    uint16_t received_crc = static_cast<uint16_t>(frame[len - 2]) |
                            (static_cast<uint16_t>(frame[len - 1]) << 8);
    uint16_t calc_crc = modbusCrc16(frame, len - 2);
    req.crc_ok = (received_crc == calc_crc);
    if (!req.crc_ok) {
        return false;
    }

    req.address = (static_cast<uint16_t>(frame[2]) << 8) | frame[3];

    if (req.function == fc::READ_HOLDING_REGISTERS) {
        if (len != 8) return false;
        req.count = (static_cast<uint16_t>(frame[4]) << 8) | frame[5];
        return true;
    }
    if (req.function == fc::WRITE_SINGLE_REGISTER) {
        if (len != 8) return false;
        req.value = (static_cast<uint16_t>(frame[4]) << 8) | frame[5];
        return true;
    }

    // Unknown function: still parsed (handler will reject)
    return true;
}

} // namespace rtu