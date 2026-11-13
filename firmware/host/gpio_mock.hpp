#pragma once

#include "firmware/core/interfaces/IGpio.hpp"

namespace rtu {

class GpioMock : public IGpio {
public:
    void setOutput(int index, bool on) override;
    bool getOutput(int index) const override;
private:
    bool outputs_[2] = {false, false};
};

} // namespace rtu