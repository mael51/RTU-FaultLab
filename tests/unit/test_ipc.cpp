#include <gtest/gtest.h>
#include "firmware/host/transport_loopback.hpp"
#include "firmware/core/registers.hpp"

using namespace rtu;

TEST(IpcPhase5, LoopbackInjectReceive) {
    TransportLoopback t;
    uint8_t data[] = {0x01, 0x03, 0x00, 0x01};
    t.inject(data, 4);
    uint8_t buf[8]{};
    std::size_t n = t.receive(buf, 8);
    EXPECT_EQ(n, 4u);
    EXPECT_EQ(buf[0], 0x01);
    EXPECT_EQ(buf[3], 0x01);
}

TEST(IpcPhase5, LoopbackSendCapture) {
    TransportLoopback t;
    uint8_t data[] = {0xAA, 0xBB};
    EXPECT_TRUE(t.send(data, 2));
    ASSERT_EQ(t.tx().size(), 2u);
    EXPECT_EQ(t.tx()[0], 0xAA);
    EXPECT_EQ(t.tx()[1], 0xBB);
}