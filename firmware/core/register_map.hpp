#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "firmware/core/registers.hpp"

namespace rtu {

class RegisterMap {
   public:
    RegisterMap();

    // Read a register by address (returns 0 if out of range).
    uint16_t read(uint16_t addr) const;

    // Write a register by address. Returns true on success.
    // On failure, sets last error and returns false.
    bool write(uint16_t addr, uint16_t value);

    // State helpers
    void setOutputState(uint16_t value);
    void setDeviceStatusBit(uint16_t bit, bool on);
    void setLastError(uint16_t code);
    void incrementUptime();
    void reset();  // SAFE state

    uint16_t deviceStatus() const { return regs_[reg::DEVICE_STATUS - reg::FIRST]; }

   private:
    std::array<uint16_t, reg::COUNT> regs_{};
};

}  // namespace rtu