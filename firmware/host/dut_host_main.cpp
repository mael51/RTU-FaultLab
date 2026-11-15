#include "firmware/core/modbus_parser.hpp"
#include "firmware/core/modbus_handler.hpp"
#include "firmware/core/register_map.hpp"
#include "firmware/core/safety_controller.hpp"
#include "firmware/core/registers.hpp"
#include "firmware/host/clock_host.hpp"
#include "firmware/host/gpio_mock.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
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
    // Real host clock based on steady_clock
    class RealClock : public IClock {
    public:
        uint32_t nowMs() const override {
            auto now = std::chrono::steady_clock::now();
            return static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
        }
    } clk;
    GpioMock gpio;
    SafetyController safety(clk, gpio, regs, cfg::HEARTBEAT_TIMEOUT_MS);

    // Thread to read stdin lines (STAB-D01: all queue access under qmtx)
    std::queue<std::string> lineQueue;
    std::mutex qmtx;
    std::condition_variable qcv;
    bool eof = false;           // guarded by qmtx (STAB-D01/D02)
    std::size_t droppedFrames = 0; // guarded by qmtx (STAB-D03)

    std::thread reader([&]() {
        std::string line;
        while (true) {
            if (!std::getline(std::cin, line)) {
                std::lock_guard<std::mutex> lk(qmtx);
                eof = true;
                qcv.notify_all();
                break;
            }
            {
                std::lock_guard<std::mutex> lk(qmtx);
                if (lineQueue.size() >= cfg::MAX_QUEUE_LINES) {
                    ++droppedFrames; // overflow policy: reject, never block
                } else {
                    lineQueue.push(line);
                }
            }
            qcv.notify_one();
        }
    });

    // Main loop: process incoming frames + periodic safety update.
    // Exits when stdin reached EOF and the queue is drained (STAB-D02).
    auto lastSafety = std::chrono::steady_clock::now();
    bool sawEof = false;
    while (true) {
        std::string line;
        bool eofNow = false;
        {
            std::unique_lock<std::mutex> lk(qmtx);
            qcv.wait_for(lk, std::chrono::milliseconds(10), [&]() {
                return !lineQueue.empty() || eof;
            });
            if (!lineQueue.empty()) {
                line = lineQueue.front();
                lineQueue.pop();
            } else if (eof) {
                eofNow = true;
            }
            if (droppedFrames > 0) {
                std::size_t n = droppedFrames;
                droppedFrames = 0;
                fprintf(stderr, "queue overflow: %zu frames dropped\n", n);
            }
        }
        if (eofNow) {
            sawEof = true;
            break;
        }

        if (!line.empty()) {
            if (line.rfind("REQ ", 0) == 0) {
                std::string hex = line.substr(4);
                auto frame = hexDecode(hex);
                if (!frame.empty()) {
                    ModbusRequest req{};
                    bool ok = parseModbusRequest(frame.data(), frame.size(), cfg::SLAVE_ADDRESS, req);
                    if (ok) {
                        uint8_t resp[256]{};
                        int n = handleModbusRequest(req, regs, resp, 256);
                        bool isException = (n >= 2) && (resp[1] & 0x80);

                        // heartbeat handling: only on valid writes
                        if (req.function == fc::WRITE_SINGLE_REGISTER) {
                            if (req.address == reg::HEARTBEAT) {
                                safety.heartbeat(req.value);
                            } else if (req.address == reg::CONTROL_WORD && !isException) {
                                if (safety.state() == SafetyState::BOOT_SAFE) {
                                    safety.heartbeat(0); // BOOT_SAFE -> READY
                                }
                                regs.write(reg::CONTROL_WORD, req.value);
                                if (safety.state() == SafetyState::READY) {
                                    safety.activate();
                                } else if (safety.state() == SafetyState::RECOVERY_PENDING ||
                                           safety.state() == SafetyState::SAFE_TIMEOUT) {
                                    safety.recover();
                                } else if (safety.state() == SafetyState::RUNNING) {
                                    safety.applyControlWord(req.value);
                                }
                                safety.heartbeat(0); // refresh window on valid command
                            }
                        }

                        safety.update();

                        if (n > 0) {
                            std::string out = "RESP " + hexEncode(resp, static_cast<std::size_t>(n));
                            std::cout << out << std::endl;
                            std::cout.flush();
                        } else {
                            fprintf(stderr, "no response\n");
                        }
                    } else {
                        fprintf(stderr, "invalid frame (crc/slave/length)\n");
                    }
                } else {
                    fprintf(stderr, "bad hex envelope\n");
                }
            } else {
                fprintf(stderr, "ignored line: %s\n", line.c_str());
            }
        }

        // Periodic safety update (~10ms)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSafety).count() >= 10) {
            safety.update();
            lastSafety = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (reader.joinable()) reader.join();
    return sawEof ? 0 : 1;
}