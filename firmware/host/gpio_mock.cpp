#include "firmware/host/gpio_mock.hpp"

namespace rtu {

void GpioMock::setOutput(int index, bool on) {
    if (index >= 0 && index < 2) outputs_[index] = on;
}

bool GpioMock::getOutput(int index) const {
    if (index >= 0 && index < 2) return outputs_[index];
    return false;
}

} // namespace rtu