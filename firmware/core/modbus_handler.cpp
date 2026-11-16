#include "firmware/core/modbus_handler.hpp"

#include <cstddef>
#include <cstdint>

#include "firmware/core/modbus_crc.hpp"
#include "firmware/core/register_map.hpp"

namespace rtu {

namespace {
void putCrc(uint8_t* buf, std::size_t len) {
    uint16_t crc = modbusCrc16(buf, len);
    buf[len] = static_cast<uint8_t>(crc & 0xFF);
    buf[len + 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);
}

// Response lengths used for capacity checks (V1-B1).
constexpr std::size_t EXCEPTION_LEN = 5;  // addr + func|0x80 + code + CRC
constexpr std::size_t FC06_LEN = 8;       // addr + func + addr + value + CRC
}  // namespace

int handleModbusRequest(const ModbusRequest& req, RegisterMap& regs, uint8_t* response,
                        std::size_t resp_capacity) {
    if (response == nullptr) return 0;

    // Exception helper: never writes beyond resp_capacity.
    const auto writeException = [&](uint8_t code) -> int {
        if (resp_capacity < EXCEPTION_LEN) return 0;
        response[0] = req.slave;
        response[1] = req.function | 0x80;
        response[2] = code;
        putCrc(response, 3);
        return static_cast<int>(EXCEPTION_LEN);
    };

    // FC03 - Read Holding Registers
    if (req.function == fc::READ_HOLDING_REGISTERS) {
        if (req.address < reg::FIRST || req.address > reg::LAST) {
            return writeException(exc::ILLEGAL_DATA_ADDRESS);
        }
        if (req.count == 0 || req.count > 125 ||
            static_cast<uint32_t>(req.address) + req.count > (reg::LAST + 1)) {
            return writeException(exc::ILLEGAL_DATA_VALUE);
        }
        std::size_t byte_count = req.count * 2;
        std::size_t total = 3 + byte_count + 2;
        if (total > resp_capacity) return 0;

        response[0] = req.slave;
        response[1] = fc::READ_HOLDING_REGISTERS;
        response[2] = static_cast<uint8_t>(byte_count);
        for (uint16_t i = 0; i < req.count; ++i) {
            uint16_t v = regs.read(req.address + i);
            response[3 + i * 2] = static_cast<uint8_t>((v >> 8) & 0xFF);
            response[3 + i * 2 + 1] = static_cast<uint8_t>(v & 0xFF);
        }
        putCrc(response, 3 + byte_count);
        return static_cast<int>(total);
    }

    // FC06 - Write Single Register
    if (req.function == fc::WRITE_SINGLE_REGISTER) {
        if (req.address < reg::FIRST || req.address > reg::LAST) {
            return writeException(exc::ILLEGAL_DATA_ADDRESS);
        }
        if (!regs.writable(req.address)) {
            return writeException(exc::ILLEGAL_DATA_ADDRESS);
        }
        // Echo response: capacity checked BEFORE any state mutation (V1-B1).
        if (resp_capacity < FC06_LEN) return 0;
        if (!regs.write(req.address, req.value)) {
            return writeException(exc::ILLEGAL_DATA_VALUE);
        }

        response[0] = req.slave;
        response[1] = fc::WRITE_SINGLE_REGISTER;
        response[2] = static_cast<uint8_t>((req.address >> 8) & 0xFF);
        response[3] = static_cast<uint8_t>(req.address & 0xFF);
        response[4] = static_cast<uint8_t>((req.value >> 8) & 0xFF);
        response[5] = static_cast<uint8_t>(req.value & 0xFF);
        putCrc(response, 6);
        return static_cast<int>(FC06_LEN);
    }

    // Unsupported function
    return writeException(exc::ILLEGAL_FUNCTION);
}

}  // namespace rtu
