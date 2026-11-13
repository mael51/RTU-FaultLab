#include "firmware/core/modbus_handler.hpp"
#include "firmware/core/modbus_crc.hpp"
#include "firmware/core/registers.hpp"
#include "firmware/core/register_map.hpp"

#include <cstddef>
#include <cstdint>

namespace rtu {

namespace {
void putCrc(uint8_t* buf, std::size_t len) {
    uint16_t crc = modbusCrc16(buf, len);
    buf[len]     = static_cast<uint8_t>(crc & 0xFF);
    buf[len + 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);
}

bool isWritable(uint16_t addr) {
    return addr == reg::CONTROL_WORD || addr == reg::HEARTBEAT;
}
} // namespace

int handleModbusRequest(const ModbusRequest& req,
                       uint16_t* regs, std::size_t reg_count,
                       uint8_t* response, std::size_t resp_capacity) {
    if (response == nullptr || regs == nullptr) return 0;

    // FC03 - Read Holding Registers
    if (req.function == fc::READ_HOLDING_REGISTERS) {
        if (req.address < reg::FIRST || (req.address + req.count) > (reg::LAST + 1) ||
            req.count == 0 || req.count > 125) {
            // exception illegal data address
            response[0] = req.slave;
            response[1] = fc::READ_HOLDING_REGISTERS | 0x80;
            response[2] = exc::ILLEGAL_DATA_ADDRESS;
            putCrc(response, 3);
            return 5;
        }
        if (req.address + req.count > reg_count + reg::FIRST) {
            response[0] = req.slave;
            response[1] = fc::READ_HOLDING_REGISTERS | 0x80;
            response[2] = exc::ILLEGAL_DATA_ADDRESS;
            putCrc(response, 3);
            return 5;
        }
        std::size_t byte_count = req.count * 2;
        std::size_t total = 3 + byte_count + 2;
        if (total > resp_capacity) return 0;

        response[0] = req.slave;
        response[1] = fc::READ_HOLDING_REGISTERS;
        response[2] = static_cast<uint8_t>(byte_count);
        for (uint16_t i = 0; i < req.count; ++i) {
            uint16_t v = regs[req.address - reg::FIRST + i];
            response[3 + i * 2]     = static_cast<uint8_t>((v >> 8) & 0xFF);
            response[3 + i * 2 + 1] = static_cast<uint8_t>(v & 0xFF);
        }
        putCrc(response, 3 + byte_count);
        return static_cast<int>(total);
    }

    // FC06 - Write Single Register
    if (req.function == fc::WRITE_SINGLE_REGISTER) {
        if (req.address < reg::FIRST || req.address > reg::LAST) {
            response[0] = req.slave;
            response[1] = fc::WRITE_SINGLE_REGISTER | 0x80;
            response[2] = exc::ILLEGAL_DATA_ADDRESS;
            putCrc(response, 3);
            return 5;
        }
        if (!isWritable(req.address)) {
            response[0] = req.slave;
            response[1] = fc::WRITE_SINGLE_REGISTER | 0x80;
            response[2] = exc::ILLEGAL_DATA_ADDRESS;
            putCrc(response, 3);
            return 5;
        }
        // Reserved bits check for CONTROL_WORD
        if (req.address == reg::CONTROL_WORD && (req.value & cw::RESERVED_MASK) != 0) {
            response[0] = req.slave;
            response[1] = fc::WRITE_SINGLE_REGISTER | 0x80;
            response[2] = exc::ILLEGAL_DATA_VALUE;
            putCrc(response, 3);
            return 5;
        }

        regs[req.address - reg::FIRST] = req.value;

        // Echo response
        response[0] = req.slave;
        response[1] = fc::WRITE_SINGLE_REGISTER;
        response[2] = static_cast<uint8_t>((req.address >> 8) & 0xFF);
        response[3] = static_cast<uint8_t>(req.address & 0xFF);
        response[4] = static_cast<uint8_t>((req.value >> 8) & 0xFF);
        response[5] = static_cast<uint8_t>(req.value & 0xFF);
        putCrc(response, 6);
        return 8;
    }

    // Unsupported function
    response[0] = req.slave;
    response[1] = req.function | 0x80;
    response[2] = exc::ILLEGAL_FUNCTION;
    putCrc(response, 3);
    return 5;
}

int handleModbusRequest(const ModbusRequest& req,
                       RegisterMap& regs,
                       uint8_t* response, std::size_t resp_capacity) {
    return handleModbusRequest(req, regs.raw(), RegisterMap::SIZE, response, resp_capacity);
}

} // namespace rtu