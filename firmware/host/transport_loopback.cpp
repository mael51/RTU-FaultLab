#include "firmware/host/transport_loopback.hpp"

namespace rtu {

bool TransportLoopback::send(const uint8_t* data, std::size_t len) {
    if (data == nullptr) return false;
    tx_.insert(tx_.end(), data, data + len);
    return true;
}

std::size_t TransportLoopback::receive(uint8_t* data, std::size_t max_len) {
    if (data == nullptr || rx_.empty()) return 0;
    std::size_t n = std::min(max_len, rx_.size());
    for (std::size_t i = 0; i < n; ++i) data[i] = rx_[i];
    rx_.erase(rx_.begin(), rx_.begin() + n);
    return n;
}

void TransportLoopback::inject(const uint8_t* data, std::size_t len) {
    if (data == nullptr) return;
    rx_.insert(rx_.end(), data, data + len);
}

} // namespace rtu