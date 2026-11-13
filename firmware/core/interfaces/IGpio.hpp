#pragma once

namespace rtu {

class IGpio {
public:
    virtual ~IGpio() = default;
    virtual void setOutput(int index, bool on) = 0;
    virtual bool getOutput(int index) const = 0;
};

} // namespace rtu