#include "firmware/core/safety_controller.hpp"

namespace rtu {

SafetyController::SafetyController(IClock& clock, IGpio& gpio, RegisterMap& regs,
                                   uint32_t timeout_ms)
    : clock_(clock),
      gpio_(gpio),
      regs_(regs),
      timeout_ms_(timeout_ms),
      last_heartbeat_ms_(0),
      state_(SafetyState::BOOT_SAFE) {
    reset();
}

void SafetyController::reset() {
    state_ = SafetyState::BOOT_SAFE;
    regs_.reset();
    regs_.setDeviceStatusBit(status::RESET_CAUSE, true);
    applySafe();
    last_heartbeat_ms_ = clock_.nowMs();
}

void SafetyController::heartbeat(uint16_t value) {
    (void)value;
    uint32_t now = clock_.nowMs();

    switch (state_) {
        case SafetyState::BOOT_SAFE:
            state_ = SafetyState::READY;
            regs_.setDeviceStatusBit(status::READY, true);
            regs_.setDeviceStatusBit(status::SAFE_TIMEOUT, false);
            last_heartbeat_ms_ = now;
            break;
        case SafetyState::READY:
            last_heartbeat_ms_ = now;
            break;
        case SafetyState::RUNNING:
            last_heartbeat_ms_ = now;
            break;
        case SafetyState::SAFE_TIMEOUT:
            // Heartbeat alone does NOT reactivate outputs.
            state_ = SafetyState::RECOVERY_PENDING;
            regs_.setDeviceStatusBit(status::SAFE_TIMEOUT, true);
            last_heartbeat_ms_ = now;
            break;
        case SafetyState::RECOVERY_PENDING:
            last_heartbeat_ms_ = now;
            break;
    }
}

void SafetyController::update() {
    if (state_ != SafetyState::RUNNING) return;
    uint32_t now = clock_.nowMs();
    uint32_t elapsed = now - last_heartbeat_ms_;
    if (elapsed > timeout_ms_) {
        applySafe();
        state_ = SafetyState::SAFE_TIMEOUT;
        regs_.setDeviceStatusBit(status::SAFE_TIMEOUT, true);
        regs_.setDeviceStatusBit(status::READY, false);
    }
}

void SafetyController::recover() {
    if (state_ == SafetyState::RECOVERY_PENDING) {
        state_ = SafetyState::RUNNING;
        regs_.setDeviceStatusBit(status::SAFE_TIMEOUT, false);
        regs_.setDeviceStatusBit(status::READY, true);
        applyOutputs(regs_.read(reg::CONTROL_WORD));
        last_heartbeat_ms_ = clock_.nowMs();
    }
}

void SafetyController::activate() {
    if (state_ == SafetyState::READY) {
        state_ = SafetyState::RUNNING;
        regs_.setDeviceStatusBit(status::READY, true);
        applyOutputs(regs_.read(reg::CONTROL_WORD));
        last_heartbeat_ms_ = clock_.nowMs();
    }
}

void SafetyController::applySafe() {
    gpio_.setOutput(0, false);
    gpio_.setOutput(1, false);
    regs_.setOutputState(0);
}

void SafetyController::applyOutputs(uint16_t cw) {
    gpio_.setOutput(0, (cw & cw::OUTPUT1) != 0);
    gpio_.setOutput(1, (cw & cw::OUTPUT2) != 0);
    regs_.setOutputState(cw & 0x0003);
}

void SafetyController::applyControlWord(uint16_t cw) {
    if (state_ == SafetyState::RUNNING) {
        applyOutputs(cw);
    }
}

}  // namespace rtu