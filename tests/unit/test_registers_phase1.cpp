#include <gtest/gtest.h>
#include "firmware/core/registers.hpp"

using namespace rtu;

TEST(RegisterMapPhase1, Addresses) {
    EXPECT_EQ(reg::CONTROL_WORD, 0x0000);
    EXPECT_EQ(reg::HEARTBEAT, 0x0005);
    EXPECT_EQ(reg::COUNT, 6);
}

TEST(RegisterMapPhase1, ControlWordBits) {
    EXPECT_EQ(cw::OUTPUT1, 0x0001);
    EXPECT_EQ(cw::OUTPUT2, 0x0002);
    EXPECT_EQ(cw::RESERVED_MASK, 0xFFFC);
}

TEST(RegisterMapPhase1, HeartbeatTimeout) {
    EXPECT_EQ(cfg::HEARTBEAT_TIMEOUT_MS, 1000u);
    EXPECT_EQ(cfg::MAX_FRAME_BYTES, 256);
    EXPECT_EQ(cfg::SLAVE_ADDRESS, 0x01);
}