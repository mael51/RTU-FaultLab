#include <gtest/gtest.h>
#include "firmware/core/safety_controller.hpp"
#include "firmware/core/register_map.hpp"
#include "firmware/host/clock_host.hpp"
#include "firmware/host/gpio_mock.hpp"
#include "firmware/core/registers.hpp"

using namespace rtu;

TEST(SafetyPhase4, BootSafe) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    EXPECT_TRUE(sc.isSafe());
    EXPECT_FALSE(gpio.getOutput(0));
    EXPECT_FALSE(gpio.getOutput(1));
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0);
}

TEST(SafetyPhase4, HeartbeatTimeout) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    sc.heartbeat(1);
    EXPECT_FALSE(sc.isSafe());
    // advance beyond timeout
    clk.advance(1500);
    sc.update();
    EXPECT_TRUE(sc.isSafe());
    EXPECT_FALSE(gpio.getOutput(0));
    EXPECT_FALSE(gpio.getOutput(1));
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0);
    EXPECT_EQ(regs.read(reg::DEVICE_STATUS) & status::SAFE_TIMEOUT, status::SAFE_TIMEOUT);
}

TEST(SafetyPhase4, Reset) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    sc.heartbeat(1);
    sc.reset();
    EXPECT_TRUE(sc.isSafe());
    EXPECT_EQ(regs.read(reg::DEVICE_STATUS) & status::RESET_CAUSE, status::RESET_CAUSE);
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0);
}

TEST(SafetyPhase4, HeartbeatKeepsSafeOff) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap rm;
    SafetyController sc(clk, gpio, rm, 1000);
    // set outputs via control word before heartbeat
    rm.write(reg::CONTROL_WORD, 0x0003);
    sc.heartbeat(1);
    EXPECT_FALSE(sc.isSafe());
    EXPECT_TRUE(gpio.getOutput(0));
    EXPECT_TRUE(gpio.getOutput(1));
    // within timeout, no safe
    clk.advance(500);
    sc.update();
    EXPECT_FALSE(sc.isSafe());
}