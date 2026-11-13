#pragma once

#include "firmware/core/interfaces/IGpio.hpp"

namespace rtu {

// STM32 implementation of IGpio using HAL GPIO.
class Stm32Gpio : public IGpio {
public:
    void setOutput(int index, bool on) override;
    bool getOutput(int index) const override;
};

} // namespace rtu