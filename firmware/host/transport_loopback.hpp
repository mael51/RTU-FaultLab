#pragma once

#include "firmware/core/interfaces/IByteTransport.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace rtu {

class TransportLoopback : public IByteTransport {
public:
    bool send(const uint8_t* data, std::size_t len) override;
    std::size_t receive(uint8_t* data, std::size_t max_len) override;

    // Test helpers
    void inject(const uint8_t* data, std::size_t len);
    const std::vector<uint8_t>& tx() const { return tx_; }

private:
    std::vector<uint8_t> rx_;
    std::vector<uint8_t> tx_;
};

} // namespace rtu