#include <gtest/gtest.h>

#include <vector>

#include "firmware/core/modbus_crc.hpp"
#include "firmware/core/modbus_handler.hpp"
#include "firmware/core/modbus_parser.hpp"
#include "firmware/core/register_map.hpp"
#include "firmware/core/registers.hpp"

using namespace rtu;

namespace {

// Build a Modbus RTU request frame with a valid CRC.
std::vector<uint8_t> buildFrame(uint8_t slave, uint8_t fn, uint16_t addr, uint16_t data,
                                uint8_t extra = 0) {
    std::vector<uint8_t> f = {slave,
                              fn,
                              static_cast<uint8_t>(addr >> 8),
                              static_cast<uint8_t>(addr & 0xFF),
                              static_cast<uint8_t>(data >> 8),
                              static_cast<uint8_t>(data & 0xFF)};
    if (extra != 0) f.push_back(extra);  // optional extra byte (length abuse tests)
    uint16_t crc = modbusCrc16(f.data(), f.size());
    f.push_back(static_cast<uint8_t>(crc & 0xFF));
    f.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    return f;
}

}  // namespace

// ---------------------------------------------------------------------------
// Parser (STAB-B01)
// ---------------------------------------------------------------------------

TEST(ModbusParser, ValidFC03) {
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, 0x0001, 0x0001);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    EXPECT_EQ(req.function, fc::READ_HOLDING_REGISTERS);
    EXPECT_EQ(req.address, 0x0001);
    EXPECT_EQ(req.count, 1);
}

TEST(ModbusParser, InvalidCrc) {
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, 0x0001, 0x0001);
    f[7] ^= 0xFF;  // corrupt CRC high byte
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f.data(), f.size(), 0x01, req));
}

TEST(ModbusParser, InvalidCrcLowByte) {
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, 0x0001, 0x0001);
    f[6] ^= 0xFF;  // corrupt CRC low byte
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f.data(), f.size(), 0x01, req));
}

TEST(ModbusParser, Truncated) {
    uint8_t f[] = {0x01, 0x03, 0x00, 0x01};
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f, 4, 0x01, req));
}

TEST(ModbusParser, TooShort) {
    uint8_t f[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0xD5};
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f, 7, 0x01, req));
}

TEST(ModbusParser, WrongSlave) {
    auto f = buildFrame(0x02, fc::READ_HOLDING_REGISTERS, 0x0001, 0x0001);
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f.data(), f.size(), 0x01, req));
}

TEST(ModbusParser, InconsistentLength) {
    // FC03 with an extra data byte before the CRC
    uint8_t data[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x00};
    uint16_t crc = modbusCrc16(data, 7);
    uint8_t f[9] = {0x01,
                    0x03,
                    0x00,
                    0x01,
                    0x00,
                    0x01,
                    0x00,
                    static_cast<uint8_t>(crc & 0xFF),
                    static_cast<uint8_t>((crc >> 8) & 0xFF)};
    ModbusRequest req{};
    EXPECT_FALSE(parseModbusRequest(f, 9, 0x01, req));
}

TEST(ModbusParser, UnsupportedFunctionParsed) {
    // Unknown function with a valid CRC is parsed (handler rejects it later).
    auto f = buildFrame(0x01, 0x05, 0x0000, 0x0000);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    EXPECT_EQ(req.function, 0x05);
}

// ---------------------------------------------------------------------------
// CRC16 reference vectors (STAB-B05)
// ---------------------------------------------------------------------------

TEST(ModbusCrc, ReferenceVectors) {
    // Known Modbus RTU vector: CRC of "01 03 00 01 00 01" = bytes D5 CA
    // (CRC register value 0xCAD5, transmitted little-endian)
    uint8_t v1[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01};
    EXPECT_EQ(modbusCrc16(v1, sizeof(v1)), 0xCAD5);
    // Determinism: same input -> same output
    uint8_t v2[] = {0x01, 0x06, 0x00, 0x00, 0x00, 0x01};
    EXPECT_EQ(modbusCrc16(v2, sizeof(v2)), modbusCrc16(v2, sizeof(v2)));
}

// ---------------------------------------------------------------------------
// Handler FC03 (STAB-B02)
// ---------------------------------------------------------------------------

TEST(ModbusHandler, FC03Read) {
    RegisterMap regs;
    regs.write(reg::HEARTBEAT, 0x1234);
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, reg::HEARTBEAT, 0x0001);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 7);  // slave+func+count+2 data+crc(2)
    EXPECT_EQ(resp[0], 0x01);
    EXPECT_EQ(resp[1], 0x03);
    EXPECT_EQ(resp[2], 0x02);
    EXPECT_EQ(resp[3], 0x12);
    EXPECT_EQ(resp[4], 0x34);
}

TEST(ModbusHandler, FC03QuantityZeroRejected) {
    RegisterMap regs;
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, 0x0000, 0x0000);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[1], fc::READ_HOLDING_REGISTERS | 0x80);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_ADDRESS);
}

TEST(ModbusHandler, FC03QuantityTooLargeRejected) {
    RegisterMap regs;
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, 0x0000, 126);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_ADDRESS);
}

TEST(ModbusHandler, FC03OutOfRangeRejected) {
    RegisterMap regs;
    // Start beyond LAST (0x0005)
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, 0x0006, 0x0001);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_ADDRESS);
}

TEST(ModbusHandler, FC03SpanExceedingRangeRejected) {
    RegisterMap regs;
    // Start at LAST, count 2 -> beyond map
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, reg::LAST, 0x0002);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_ADDRESS);
}

TEST(ModbusHandler, FC03FullRangeOk) {
    RegisterMap regs;
    // Read all 6 registers starting at FIRST
    auto f = buildFrame(0x01, fc::READ_HOLDING_REGISTERS, reg::FIRST, reg::COUNT);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 3 + reg::COUNT * 2 + 2);
    EXPECT_EQ(resp[2], reg::COUNT * 2);
}

// ---------------------------------------------------------------------------
// Handler FC06 (STAB-B03)
// ---------------------------------------------------------------------------

TEST(ModbusHandler, FC06WriteControlWordEcho) {
    RegisterMap regs;
    auto f = buildFrame(0x01, fc::WRITE_SINGLE_REGISTER, reg::CONTROL_WORD, 0x0001);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 8);
    EXPECT_EQ(resp[2], 0x00);
    EXPECT_EQ(resp[3], 0x00);
    EXPECT_EQ(resp[4], 0x00);
    EXPECT_EQ(resp[5], 0x01);
    EXPECT_EQ(regs.read(reg::CONTROL_WORD), 0x0001);
}

TEST(ModbusHandler, FC06WriteRORejected) {
    RegisterMap regs;
    // Write to OUTPUT_STATE (0x0001) which is RO
    auto f = buildFrame(0x01, fc::WRITE_SINGLE_REGISTER, reg::OUTPUT_STATE, 0x0001);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);  // exception
    EXPECT_EQ(resp[1], fc::WRITE_SINGLE_REGISTER | 0x80);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_ADDRESS);
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0x0000);  // unchanged
}

TEST(ModbusHandler, FC06ReservedBitsRejected) {
    RegisterMap regs;
    // CONTROL_WORD with reserved bit (0x0004)
    auto f = buildFrame(0x01, fc::WRITE_SINGLE_REGISTER, reg::CONTROL_WORD, 0x0004);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_VALUE);
    EXPECT_EQ(regs.read(reg::CONTROL_WORD), 0x0000);  // unchanged
}

TEST(ModbusHandler, FC06WriteBeyondRangeRejected) {
    RegisterMap regs;
    auto f = buildFrame(0x01, fc::WRITE_SINGLE_REGISTER, reg::LAST + 1, 0x0001);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[2], exc::ILLEGAL_DATA_ADDRESS);
}

TEST(ModbusHandler, BurstRequests) {
    RegisterMap regs;
    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            auto f = buildFrame(0x01, fc::WRITE_SINGLE_REGISTER, reg::CONTROL_WORD, 0x0001);
            ModbusRequest req{};
            ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
            uint8_t resp[256]{};
            handleModbusRequest(req, regs, resp, 256);
        } else {
            uint8_t f[] = {0x01, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
            ModbusRequest req{};
            EXPECT_FALSE(parseModbusRequest(f, 8, 0x01, req));
        }
    }
    // state should be consistent
    EXPECT_EQ(regs.read(reg::CONTROL_WORD), 0x0001);  // last valid write
}

TEST(ModbusHandler, UnsupportedFunction) {
    RegisterMap regs;
    // FC 0x05 not supported
    auto f = buildFrame(0x01, 0x05, 0x0000, 0x0000);
    ModbusRequest req{};
    ASSERT_TRUE(parseModbusRequest(f.data(), f.size(), 0x01, req));
    uint8_t resp[256]{};
    int n = handleModbusRequest(req, regs, resp, 256);
    EXPECT_EQ(n, 5);
    EXPECT_EQ(resp[1], 0x85);
    EXPECT_EQ(resp[2], exc::ILLEGAL_FUNCTION);
}
