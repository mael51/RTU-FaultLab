#pragma once

#include "firmware/core/interfaces/IClock.hpp"

namespace rtu {

// STM32 implementation of IClock using SysTick / HAL_GetTick.
class Stm32Clock : public IClock {
public:
    uint32_t nowMs() const override;
};

} // namespace rtu