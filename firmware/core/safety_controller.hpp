#pragma once

#include <cstdint>

#include "firmware/core/interfaces/IClock.hpp"
#include "firmware/core/interfaces/IGpio.hpp"
#include "firmware/core/register_map.hpp"
#include "firmware/core/registers.hpp"

namespace rtu {

enum class SafetyState {
    BOOT_SAFE,
    READY,
    RUNNING,
    SAFE_TIMEOUT,
    RECOVERY_PENDING,
};

class SafetyController {
   public:
    SafetyController(IClock& clock, IGpio& gpio, RegisterMap& regs,
                     uint32_t timeout_ms = cfg::HEARTBEAT_TIMEOUT_MS);

    void reset();                    // SAFE state, set RESET_CAUSE
    void heartbeat(uint16_t value);  // refresh window / state transition
    void update();                   // check timeout, apply SAFE if expired
    void recover();                  // explicit recovery from SAFE_TIMEOUT
    void activate();                 // transition READY -> RUNNING and apply CONTROL_WORD
    void applyControlWord(
        uint16_t cw);  // apply outputs from a validated CONTROL_WORD write (RUNNING only)

    bool isSafe() const {
        return state_ == SafetyState::BOOT_SAFE || state_ == SafetyState::READY ||
               state_ == SafetyState::SAFE_TIMEOUT || state_ == SafetyState::RECOVERY_PENDING;
    }
    SafetyState state() const { return state_; }

   private:
    IClock& clock_;
    IGpio& gpio_;
    RegisterMap& regs_;
    uint32_t timeout_ms_;
    uint32_t last_heartbeat_ms_;
    SafetyState state_;

    void applySafe();
    void applyOutputs(uint16_t cw);
};

}  // namespace rtu