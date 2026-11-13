#include "firmware/core/modbus_parser.hpp"
#include "firmware/core/modbus_handler.hpp"
#include "firmware/core/register_map.hpp"
#include "firmware/core/safety_controller.hpp"
#include "firmware/core/registers.hpp"
#include "firmware/host/clock_host.hpp"
#include "firmware/host/gpio_mock.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace rtu;

static std::vector<uint8_t> hexDecode(const std::string& s) {
    std::vector<uint8_t> out;
    std::string t;
    for (char c : s) if (c != ' ') t.push_back(c);
    if (t.size() % 2 != 0) return out;
    for (std::size_t i = 0; i < t.size(); i += 2) {
        unsigned int b = 0;
        sscanf(t.c_str() + i, "%2x", &b);
        out.push_back(static_cast<uint8_t>(b));
    }
    return out;
}

static std::string hexEncode(const uint8_t* data, std::size_t len) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < len; ++i) {
        if (i) oss << ' ';
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", data[i]);
        oss << buf;
    }
    return oss.str();
}

int main() {
    RegisterMap regs;
    ClockHost clk(0);
    GpioMock gpio;
    SafetyController safety(clk, gpio, regs, cfg::HEARTBEAT_TIMEOUT_MS);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.rfind("REQ ", 0) != 0) {
            // ignore / log
            fprintf(stderr, "ignored line: %s\n", line.c_str());
            continue;
        }
        std::string hex = line.substr(4);
        auto frame = hexDecode(hex);
        if (frame.empty()) {
            fprintf(stderr, "bad hex envelope\n");
            continue;
        }

        ModbusRequest req{};
        bool ok = parseModbusRequest(frame.data(), frame.size(), cfg::SLAVE_ADDRESS, req);
        if (!ok) {
            fprintf(stderr, "invalid frame (crc/slave/length)\n");
            continue;
        }

        uint8_t resp[256]{};
        int n = handleModbusRequest(req, regs, resp, 256);

        // heartbeat handling
        if (req.function == fc::WRITE_SINGLE_REGISTER && req.address == reg::HEARTBEAT) {
            safety.heartbeat(req.value);
        }
        // control word application
        if (req.function == fc::WRITE_SINGLE_REGISTER && req.address == reg::CONTROL_WORD) {
            safety.heartbeat(0); // refresh window on any valid command
        }
        safety.update();

        if (n > 0) {
            std::string out = "RESP " + hexEncode(resp, static_cast<std::size_t>(n));
            std::cout << out << std::endl;
        } else {
            fprintf(stderr, "no response\n");
        }
    }
    return 0;
}