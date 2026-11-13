#pragma once

#include "firmware/core/interfaces/IClock.hpp"

#include <cstdint>

namespace rtu {

class ClockHost : public IClock {
public:
    ClockHost(uint32_t start_ms = 0);
    uint32_t nowMs() const override;
    void advance(uint32_t ms);
    void set(uint32_t ms);
private:
    uint32_t now_;
};

} // namespace rtu