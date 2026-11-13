#include <gtest/gtest.h>
#include "firmware/core/register_map.hpp"
#include "firmware/core/registers.hpp"

using namespace rtu;

TEST(RegisterMapPhase3, BootSafe) {
    RegisterMap rm;
    EXPECT_EQ(rm.read(reg::CONTROL_WORD), 0);
    EXPECT_EQ(rm.read(reg::OUTPUT_STATE), 0);
    EXPECT_EQ(rm.read(reg::DEVICE_STATUS) & status::READY, status::READY);
    EXPECT_EQ(rm.read(reg::LAST_ERROR), diag::NONE);
}

TEST(RegisterMapPhase3, WriteControlWord) {
    RegisterMap rm;
    EXPECT_TRUE(rm.write(reg::CONTROL_WORD, 0x0003));
    EXPECT_EQ(rm.read(reg::CONTROL_WORD), 0x0003);
    EXPECT_EQ(rm.read(reg::OUTPUT_STATE), 0x0003);
    EXPECT_EQ(rm.read(reg::LAST_ERROR), diag::NONE);
}

TEST(RegisterMapPhase3, WriteReservedBitsRejected) {
    RegisterMap rm;
    EXPECT_FALSE(rm.write(reg::CONTROL_WORD, 0x0004));
    EXPECT_EQ(rm.read(reg::CONTROL_WORD), 0);
    EXPECT_EQ(rm.read(reg::OUTPUT_STATE), 0);
    EXPECT_EQ(rm.read(reg::LAST_ERROR), diag::ILLEGAL_VALUE);
}

TEST(RegisterMapPhase3, WriteReadOnlyRejected) {
    RegisterMap rm;
    EXPECT_FALSE(rm.write(reg::OUTPUT_STATE, 0x0001));
    EXPECT_EQ(rm.read(reg::LAST_ERROR), diag::ILLEGAL_VALUE);
}

TEST(RegisterMapPhase3, UptimeIncrement) {
    RegisterMap rm;
    rm.incrementUptime();
    rm.incrementUptime();
    EXPECT_EQ(rm.read(reg::UPTIME_SECONDS), 2);
}

TEST(RegisterMapPhase3, Reset) {
    RegisterMap rm;
    rm.write(reg::CONTROL_WORD, 0x0003);
    rm.reset();
    EXPECT_EQ(rm.read(reg::CONTROL_WORD), 0);
    EXPECT_EQ(rm.read(reg::OUTPUT_STATE), 0);
    EXPECT_EQ(rm.read(reg::DEVICE_STATUS) & status::READY, status::READY);
}