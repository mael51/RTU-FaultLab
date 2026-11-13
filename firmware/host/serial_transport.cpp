#include "firmware/host/serial_transport.hpp"

#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace rtu {

SerialTransport::SerialTransport(const std::string& port, uint32_t baud)
    : port_(port), baud_(baud), handle_(nullptr) {}

SerialTransport::~SerialTransport() { close(); }

bool SerialTransport::open() {
#if defined(_WIN32)
    std::string path = "\\\\.\\" + port_;
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) { CloseHandle(h); return false; }
    dcb.BaudRate = baud_;
    dcb.ByteSize = 8;
    dcb.Parity = EVENPARITY;
    dcb.StopBits = ONESTOPBIT;
    if (!SetCommState(h, &dcb)) { CloseHandle(h); return false; }
    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout = 50;
    to.ReadTotalTimeoutConstant = 50;
    to.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &to);
    handle_ = h;
    return true;
#else
    int fd = open(port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) return false;
    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) { ::close(fd); return false; }
    cfsetospeed(&tty, baud_);
    cfsetispeed(&tty, baud_);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= PARENB;
    tty.c_cflag &= ~PARODD;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5; // 0.5s
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { ::close(fd); return false; }
    handle_ = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
    return true;
#endif
}

void SerialTransport::close() {
#if defined(_WIN32)
    if (handle_) { CloseHandle(static_cast<HANDLE>(handle_)); handle_ = nullptr; }
#else
    if (handle_) { ::close(static_cast<int>(reinterpret_cast<intptr_t>(handle_))); handle_ = nullptr; }
#endif
}

bool SerialTransport::send(const uint8_t* data, std::size_t len) {
    if (!handle_ || !data) return false;
#if defined(_WIN32)
    DWORD written = 0;
    return WriteFile(static_cast<HANDLE>(handle_), data, static_cast<DWORD>(len), &written, nullptr) &&
           written == len;
#else
    ssize_t w = ::write(static_cast<int>(reinterpret_cast<intptr_t>(handle_)), data, len);
    return w == static_cast<ssize_t>(len);
#endif
}

std::size_t SerialTransport::receive(uint8_t* data, std::size_t max_len) {
    if (!handle_ || !data || max_len == 0) return 0;
#if defined(_WIN32)
    DWORD read = 0;
    if (!ReadFile(static_cast<HANDLE>(handle_), data, static_cast<DWORD>(max_len), &read, nullptr)) return 0;
    return read;
#else
    ssize_t r = ::read(static_cast<int>(reinterpret_cast<intptr_t>(handle_)), data, max_len);
    if (r < 0) return 0;
    return static_cast<std::size_t>(r);
#endif
}

} // namespace rtu