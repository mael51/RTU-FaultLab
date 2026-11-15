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
    EXPECT_EQ(sc.state(), SafetyState::BOOT_SAFE);
}

TEST(SafetyPhase4, HeartbeatReady) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    sc.heartbeat(1);
    EXPECT_TRUE(sc.isSafe()); // READY is safe
    EXPECT_EQ(sc.state(), SafetyState::READY);
    EXPECT_FALSE(gpio.getOutput(0));
}

TEST(SafetyPhase4, ActivateRunning) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    regs.write(reg::CONTROL_WORD, 0x0003);
    sc.heartbeat(1); // BOOT_SAFE -> READY
    sc.activate();     // READY -> RUNNING
    EXPECT_FALSE(sc.isSafe());
    EXPECT_TRUE(gpio.getOutput(0));
    EXPECT_TRUE(gpio.getOutput(1));
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0x0003);
}

TEST(SafetyPhase4, HeartbeatTimeout) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    regs.write(reg::CONTROL_WORD, 0x0003);
    sc.heartbeat(1);
    sc.activate();
    EXPECT_FALSE(sc.isSafe());
    clk.advance(1500);
    sc.update();
    EXPECT_TRUE(sc.isSafe());
    EXPECT_EQ(sc.state(), SafetyState::SAFE_TIMEOUT);
    EXPECT_FALSE(gpio.getOutput(0));
    EXPECT_FALSE(gpio.getOutput(1));
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0);
}

TEST(SafetyPhase4, NoAutomaticReactivation) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    regs.write(reg::CONTROL_WORD, 0x0003);
    sc.heartbeat(1);
    sc.activate();
    EXPECT_FALSE(sc.isSafe());
    clk.advance(1500);
    sc.update();
    EXPECT_TRUE(sc.isSafe());
    EXPECT_FALSE(gpio.getOutput(0));
    // heartbeat alone does NOT reactivate
    sc.heartbeat(2);
    EXPECT_TRUE(sc.isSafe());
    EXPECT_EQ(sc.state(), SafetyState::RECOVERY_PENDING);
    EXPECT_FALSE(gpio.getOutput(0));
    // explicit recover via control word
    regs.write(reg::CONTROL_WORD, 0x0003);
    sc.recover();
    EXPECT_FALSE(sc.isSafe());
    EXPECT_EQ(sc.state(), SafetyState::RUNNING);
    EXPECT_TRUE(gpio.getOutput(0));
}

TEST(SafetyPhase4, Reset) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    sc.reset();
    EXPECT_TRUE(sc.isSafe());
    EXPECT_EQ(sc.state(), SafetyState::BOOT_SAFE);
    EXPECT_EQ(regs.read(reg::DEVICE_STATUS) & status::RESET_CAUSE, status::RESET_CAUSE);
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0);
}

TEST(SafetyPhase4, ControlWordUpdateWhileRunning) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    regs.write(reg::CONTROL_WORD, 0x0003);
    sc.heartbeat(1); // BOOT_SAFE -> READY
    sc.activate();   // READY -> RUNNING, outputs ON
    EXPECT_TRUE(gpio.getOutput(0));
    EXPECT_TRUE(gpio.getOutput(1));
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0x0003);
    // update control word while RUNNING
    regs.write(reg::CONTROL_WORD, 0x0001);
    sc.applyControlWord(0x0001);
    EXPECT_TRUE(gpio.getOutput(0));
    EXPECT_FALSE(gpio.getOutput(1));
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0x0001);
}

TEST(SafetyPhase4, InvalidControlWordDoesNotApplyOutputs) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    // reserved bits rejected by RegisterMap: write fails, nothing applied
    EXPECT_FALSE(regs.write(reg::CONTROL_WORD, 0x0004));
    EXPECT_EQ(regs.read(reg::CONTROL_WORD), 0x0000);
    sc.applyControlWord(0x0004);
    EXPECT_EQ(sc.state(), SafetyState::BOOT_SAFE);
    EXPECT_FALSE(gpio.getOutput(0));
    EXPECT_FALSE(gpio.getOutput(1));
    EXPECT_EQ(regs.read(reg::OUTPUT_STATE), 0x0000);
}

TEST(SafetyPhase4, HeartbeatKeepsSafeOff) {
    ClockHost clk(0);
    GpioMock gpio;
    RegisterMap regs;
    SafetyController sc(clk, gpio, regs, 1000);
    regs.write(reg::CONTROL_WORD, 0x0003);
    sc.heartbeat(1); // BOOT_SAFE -> READY, outputs stay OFF
    EXPECT_TRUE(sc.isSafe());
    EXPECT_FALSE(gpio.getOutput(0));
    EXPECT_FALSE(gpio.getOutput(1));
    sc.activate(); // READY -> RUNNING, outputs ON
    EXPECT_FALSE(sc.isSafe());
    EXPECT_TRUE(gpio.getOutput(0));
    EXPECT_TRUE(gpio.getOutput(1));
    clk.advance(500);
    sc.update();
    EXPECT_FALSE(sc.isSafe());
}