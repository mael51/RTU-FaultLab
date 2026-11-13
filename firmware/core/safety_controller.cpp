#include "firmware/core/safety_controller.hpp"

namespace rtu {

SafetyController::SafetyController(IClock& clock, IGpio& gpio, RegisterMap& regs,
                                 uint32_t timeout_ms)
    : clock_(clock), gpio_(gpio), regs_(regs),
      timeout_ms_(timeout_ms), last_heartbeat_ms_(0),
      safe_(true), started_(false) {
    reset();
}

void SafetyController::reset() {
    safe_ = true;
    started_ = false;
    regs_.reset();
    regs_.setDeviceStatusBit(status::RESET_CAUSE, true);
    applySafe();
    last_heartbeat_ms_ = clock_.nowMs();
}

void SafetyController::heartbeat(uint16_t value) {
    (void)value;
    last_heartbeat_ms_ = clock_.nowMs();
    safe_ = false;
    started_ = true;
    regs_.setDeviceStatusBit(status::SAFE_TIMEOUT, false);
    regs_.setDeviceStatusBit(status::READY, true);
    // apply outputs from CONTROL_WORD
    applyOutputs(regs_.read(reg::CONTROL_WORD));
}

void SafetyController::update() {
    if (!started_) return; // before first heartbeat, stay safe
    uint32_t now = clock_.nowMs();
    uint32_t elapsed = now - last_heartbeat_ms_;
    if (elapsed > timeout_ms_) {
        applySafe();
        safe_ = true;
        regs_.setDeviceStatusBit(status::SAFE_TIMEOUT, true);
        regs_.setDeviceStatusBit(status::READY, false);
    }
}

void SafetyController::applySafe() {
    gpio_.setOutput(0, false);
    gpio_.setOutput(1, false);
    regs_.setOutputState(0);
    safe_ = true;
}

void SafetyController::applyOutputs(uint16_t cw) {
    gpio_.setOutput(0, (cw & cw::OUTPUT1) != 0);
    gpio_.setOutput(1, (cw & cw::OUTPUT2) != 0);
    regs_.setOutputState(cw & 0x0003);
}

} // namespace rtu