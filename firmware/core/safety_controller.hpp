#pragma once

#include "firmware/core/interfaces/IClock.hpp"
#include "firmware/core/interfaces/IGpio.hpp"
#include "firmware/core/register_map.hpp"
#include "firmware/core/registers.hpp"

#include <cstdint>

namespace rtu {

class SafetyController {
public:
    SafetyController(IClock& clock, IGpio& gpio, RegisterMap& regs,
                     uint32_t timeout_ms = cfg::HEARTBEAT_TIMEOUT_MS);

    void reset();          // SAFE state, set RESET_CAUSE
    void heartbeat(uint16_t value); // refresh window
    void update();         // check timeout, apply SAFE if expired

    bool isSafe() const { return safe_; }

private:
    IClock& clock_;
    IGpio& gpio_;
    RegisterMap& regs_;
    uint32_t timeout_ms_;
    uint32_t last_heartbeat_ms_;
    bool safe_;
    bool started_;

    void applySafe();
    void applyOutputs(uint16_t cw);
};

} // namespace rtu