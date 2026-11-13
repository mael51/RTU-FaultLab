#include "firmware/stm32/stm32_clock.hpp"

// TODO: implement using HAL_GetTick() or SysTick.
namespace rtu {
uint32_t Stm32Clock::nowMs() const { return 0; }
} // namespace rtu