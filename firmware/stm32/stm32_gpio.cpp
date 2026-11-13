#include "firmware/stm32/stm32_gpio.hpp"

// TODO: implement using HAL_GPIO_WritePin / ReadPin.
namespace rtu {
void Stm32Gpio::setOutput(int index, bool on) { (void)index; (void)on; }
bool Stm32Gpio::getOutput(int index) const { (void)index; return false; }
} // namespace rtu