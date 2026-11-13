#include "firmware/core/register_map.hpp"

namespace {

bool isWritableAddr(uint16_t addr) {
    return addr == rtu::reg::CONTROL_WORD || addr == rtu::reg::HEARTBEAT;
}

} // namespace

namespace rtu {

RegisterMap::RegisterMap() {
    reset();
}

void RegisterMap::reset() {
    regs_.fill(0);
    regs_[reg::DEVICE_STATUS - reg::FIRST] = status::READY;
}

uint16_t RegisterMap::read(uint16_t addr) const {
    if (addr < reg::FIRST || addr > reg::LAST) return 0;
    return regs_[addr - reg::FIRST];
}

bool RegisterMap::write(uint16_t addr, uint16_t value) {
    if (addr < reg::FIRST || addr > reg::LAST) {
        setLastError(diag::BAD_ADDRESS);
        return false;
    }
    if (!isWritableAddr(addr)) {
        setLastError(diag::ILLEGAL_VALUE);
        return false;
    }
    if (addr == reg::CONTROL_WORD && (value & cw::RESERVED_MASK) != 0) {
        setLastError(diag::ILLEGAL_VALUE);
        return false;
    }

    regs_[addr - reg::FIRST] = value;

    if (addr == reg::CONTROL_WORD) {
        setOutputState(value & 0x0003);
    }
    setLastError(diag::NONE);
    return true;
}

uint16_t* RegisterMap::raw() { return regs_.data(); }
const uint16_t* RegisterMap::raw() const { return regs_.data(); }

void RegisterMap::setOutputState(uint16_t value) {
    regs_[reg::OUTPUT_STATE - reg::FIRST] = value & 0x0003;
}

void RegisterMap::setDeviceStatusBit(uint16_t bit, bool on) {
    if (on) regs_[reg::DEVICE_STATUS - reg::FIRST] |= bit;
    else    regs_[reg::DEVICE_STATUS - reg::FIRST] &= ~bit;
}

void RegisterMap::setLastError(uint16_t code) {
    regs_[reg::LAST_ERROR - reg::FIRST] = code;
}

void RegisterMap::incrementUptime() {
    uint16_t& u = regs_[reg::UPTIME_SECONDS - reg::FIRST];
    if (u != 0xFFFF) ++u;
}

} // namespace rtu