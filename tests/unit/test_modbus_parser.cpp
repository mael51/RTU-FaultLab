#include <array>
#include <gtest/gtest.h>
#include "firmware/core/modbus_parser.hpp"
#include "firmware/core/modbus_handler.hpp"
#include "firmware/core/modbus_crc.hpp"
#include "firmware/core/registers.hpp"

using namespace rtu;

static std::array<uint16_t, 6> makeRegs() {
    std::array<uint16_t, 6> r{};
    r[0] = 0x0000; // CONTROL_WORD
    r[1] = 0x0000; // OUTPUT_STATE
    r[2] = 0x0001; // DEVICE_STATUS READY
    r[3] = 0;      // UPTIME
    r[4] = 0;      // LAST_ERROR
    r[5] = 0;      // HEARTBEAT
    return r;
}

TEST(ModbusParser, ValidFC03) {
    // Recompute properly:
    uint8_t data[] = {0x01,0x03,0x00,0x01,0x00,0x01};
    uint16_t crc = modbusCrc16(data, 6);
    uint8_t f[8] = {0x01,0x03,0x00,0x01,0x00,0x01,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc>>8)&0xFF)};
    ModbusRequest req{};
    EXPECT_TRUE(parseModbusRequest(f, 8, 0x01, req));
    EXPECT_EQ(req.function, fc::READ_HOLDING_REGISTERS);
    EXPECT_EQ(req.address, 0x0001);
    EXPECT_EQ(req.count, 1);
}

TEST(ModbusParser, InvalidCrc) {
    uint8_t f[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f, 8, 0x01, req));
}

TEST(ModbusParser, Truncated) {
    uint8_t f[] = {0x01, 0x03, 0x00, 0x01};
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f, 4, 0x01, req));
}

TEST(ModbusParser, WrongSlave) {
    uint8_t data[] = {0x02,0x03,0x00,0x01,0x00,0x01};
    uint16_t crc = modbusCrc16(data, 6);
    uint8_t f[8] = {0x02,0x03,0x00,0x01,0x00,0x01,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc>>8)&0xFF)};
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f, 8, 0x01, req));
}

TEST(ModbusHandler, FC03Read) {
    auto regs = makeRegs();
    regs[1] = 0x1234;
    uint8_t data[] = {0x01,0x03,0x00,0x01,0x00,0x01};
    uint16_t crc = modbusCrc16(data, 6);
    uint8_t f[8] = {0x01,0x03,0x00,0x01,0x00,0x01,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc>>8)&0xFF)};
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f, 8, 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs.data(), regs.size(), resp, 256);
    EXPECT_EQ(n, 7); // slave+func+count(1)+2 data+crc(2) = 7? 1+1+1+2+2=7 yes
    EXPECT_EQ(resp[0], 0x01);
    EXPECT_EQ(resp[1], 0x03);
    EXPECT_EQ(resp[2], 0x02);
    EXPECT_EQ(resp[3], 0x12);
    EXPECT_EQ(resp[4], 0x34);
}

TEST(ModbusHandler, FC06WriteRO) {
    auto regs = makeRegs();
    // Write to OUTPUT_STATE (0x0001) which is RO
    uint8_t data[] = {0x01,0x06,0x00,0x01,0x00,0x01};
    uint16_t crc = modbusCrc16(data, 6);
    uint8_t f[8] = {0x01,0x06,0x00,0x01,0x00,0x01,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc>>8)&0xFF)};
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f, 8, 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs.data(), regs.size(), resp, 256);
    EXPECT_EQ(n, 5); // exception
    EXPECT_EQ(resp[1], 0x86); // 0x06 | 0x80
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_ADDRESS);
    EXPECT_EQ(regs[1], 0x0000); // unchanged
}

TEST(ModbusHandler, FC06ReservedBits) {
    auto regs = makeRegs();
    // CONTROL_WORD with reserved bit (0x0004)
    uint8_t data[] = {0x01,0x06,0x00,0x00,0x00,0x04};
    uint16_t crc = modbusCrc16(data, 6);
    uint8_t f[8] = {0x01,0x06,0x00,0x00,0x00,0x04,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc>>8)&0xFF)};
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f, 8, 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs.data(), regs.size(), resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_VALUE);
    EXPECT_EQ(regs[0], 0x0000);
}

TEST(ModbusParser, InconsistentLength) {
    // FC03 with extra byte
    uint8_t data[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00};
    uint16_t crc = modbusCrc16(data, 7);
    uint8_t f[9] = {0x01,0x03,0x00,0x01,0x00,0x01,0x00,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc>>8)&0xFF)};
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f, 9, 0x01, req));
}

TEST(ModbusHandler, BurstRequests) {
    auto regs = makeRegs();
    // alternate valid write and invalid CRC
    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            uint8_t data[] = {0x01,0x06,0x00,0x00,0x00,0x01};
            uint16_t crc = modbusCrc16(data, 6);
            uint8_t f[8] = {0x01,0x06,0x00,0x00,0x00,0x01,
                            static_cast<uint8_t>(crc & 0xFF),
                            static_cast<uint8_t>((crc>>8)&0xFF)};
            ModbusRequest req{};
            ASSERT_TRUE(parseModbusRequest(f, 8, 0x01, req));
            uint8_t resp[256]{};
            handleModbusRequest(req, regs.data(), regs.size(), resp, 256);
        } else {
            uint8_t f[] = {0x01, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
            ModbusRequest req{};
            EXPECT_FALSE(parseModbusRequest(f, 8, 0x01, req));
        }
    }
    // state should be consistent
    EXPECT_EQ(regs[0], 0x0001); // last valid write
}

TEST(ModbusHandler, UnsupportedFunction) {
    // FC 0x05 not supported
    uint8_t data[] = {0x01,0x05,0x00,0x00,0x00,0x00};
    uint16_t crc = modbusCrc16(data, 6);
    uint8_t f[8] = {0x01,0x05,0x00,0x00,0x00,0x00,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc>>8)&0xFF)};
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f, 8, 0x01, req));
    auto regs = makeRegs();
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs.data(), regs.size(), resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[1], 0x85);
    EXPECT_EQ(resp[2], exc::ILLEGAL_FUNCTION);
}