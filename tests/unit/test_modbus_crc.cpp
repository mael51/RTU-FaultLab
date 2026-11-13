#include <gtest/gtest.h>
#include "firmware/core/modbus_crc.hpp"

using namespace rtu;

TEST(ModbusCrc, KnownVector) {
    // Modbus CRC of 01 03 00 00 00 01 should be 84 0A
    uint8_t data[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
    uint16_t crc = modbusCrc16(data, 6);
    EXPECT_EQ(crc, 0x0A84);
}

TEST(ModbusCrc, Empty) {
    EXPECT_EQ(modbusCrc16(nullptr, 0), 0xFFFF);
}