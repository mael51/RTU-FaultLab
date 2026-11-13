#pragma once

#include <cstdint>

namespace rtu {

class IClock {
public:
    virtual ~IClock() = default;
    virtual uint32_t nowMs() const = 0;
};

} // namespace rtu