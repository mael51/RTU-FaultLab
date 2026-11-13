#include "firmware/stm32/stm32_transport.hpp"

// TODO: implement using HAL_UART (RS-485 DE/RE control).
namespace rtu {
bool Stm32Transport::send(const uint8_t* data, std::size_t len) { (void)data; (void)len; return false; }
std::size_t Stm32Transport::receive(uint8_t* data, std::size_t max_len) { (void)data; (void)max_len; return 0; }
} // namespace rtu