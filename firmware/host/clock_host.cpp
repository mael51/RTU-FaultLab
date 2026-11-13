#include "firmware/host/clock_host.hpp"

namespace rtu {

ClockHost::ClockHost(uint32_t start_ms) : now_(start_ms) {}
uint32_t ClockHost::nowMs() const { return now_; }
void ClockHost::advance(uint32_t ms) { now_ += ms; }
void ClockHost::set(uint32_t ms) { now_ = ms; }

} // namespace rtu