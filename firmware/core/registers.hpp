#pragma once

#include <cstdint>

namespace rtu {

// ---------------------------------------------------------------------------
// Register map (Modbus holding registers) - single source of truth
// ---------------------------------------------------------------------------
namespace reg {
constexpr uint16_t CONTROL_WORD      = 0x0000; // R/W via FC06
constexpr uint16_t OUTPUT_STATE     = 0x0001; // RO via FC03
constexpr uint16_t DEVICE_STATUS    = 0x0002; // RO via FC03
constexpr uint16_t UPTIME_SECONDS   = 0x0003; // RO via FC03
constexpr uint16_t LAST_ERROR       = 0x0004; // RO via FC03
constexpr uint16_t HEARTBEAT        = 0x0005; // WO via FC06

constexpr uint16_t FIRST = CONTROL_WORD;
constexpr uint16_t LAST  = HEARTBEAT;
constexpr uint16_t COUNT = LAST - FIRST + 1;
} // namespace reg

// ---------------------------------------------------------------------------
// CONTROL_WORD bits
// ---------------------------------------------------------------------------
namespace cw {
constexpr uint16_t OUTPUT1 = (1u << 0);
constexpr uint16_t OUTPUT2 = (1u << 1);
constexpr uint16_t RESERVED_MASK = ~(OUTPUT1 | OUTPUT2);
} // namespace cw

// ---------------------------------------------------------------------------
// DEVICE_STATUS bits
// ---------------------------------------------------------------------------
namespace status {
constexpr uint16_t READY          = (1u << 0);
constexpr uint16_t SAFE_TIMEOUT   = (1u << 1);
constexpr uint16_t PROTOCOL_ERROR  = (1u << 2);
constexpr uint16_t RESET_CAUSE     = (1u << 3);
} // namespace status

// ---------------------------------------------------------------------------
// Modbus function codes (supported subset)
// ---------------------------------------------------------------------------
namespace fc {
constexpr uint8_t READ_HOLDING_REGISTERS = 0x03;
constexpr uint8_t WRITE_SINGLE_REGISTER   = 0x06;
} // namespace fc

// ---------------------------------------------------------------------------
// Modbus exception codes
// ---------------------------------------------------------------------------
namespace exc {
constexpr uint8_t ILLEGAL_FUNCTION    = 0x01;
constexpr uint8_t ILLEGAL_DATA_ADDRESS = 0x02;
constexpr uint8_t ILLEGAL_DATA_VALUE   = 0x03;
} // namespace exc

// ---------------------------------------------------------------------------
// Diagnostic error codes (LAST_ERROR)
// ---------------------------------------------------------------------------
namespace diag {
constexpr uint16_t NONE              = 0x0000;
constexpr uint16_t CRC_ERROR         = 0x0001;
constexpr uint16_t BAD_LENGTH        = 0x0002;
constexpr uint16_t BAD_ADDRESS       = 0x0003;
constexpr uint16_t ILLEGAL_FUNCTION  = 0x0004;
constexpr uint16_t ILLEGAL_VALUE     = 0x0005;
constexpr uint16_t TIMEOUT           = 0x0006;
} // namespace diag

// ---------------------------------------------------------------------------
// Transport / timing defaults
// ---------------------------------------------------------------------------
namespace cfg {
constexpr uint32_t BAUD_RATE        = 19200;
constexpr uint8_t  DATA_BITS        = 8;
constexpr char     PARITY           = 'E'; // Even
constexpr uint8_t  STOP_BITS        = 1;
constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 1000;
constexpr uint16_t MAX_FRAME_BYTES  = 256;
constexpr uint8_t  SLAVE_ADDRESS    = 0x01;
} // namespace cfg

} // namespace rtu