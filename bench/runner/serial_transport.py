"""Serial transport wrapper using PySerial."""

try:
    import serial
except ImportError:
    serial = None


class SerialTransport:
    def __init__(self, port: str, baudrate: int = 19200, timeout: float = 0.5):
        if serial is None:
            raise RuntimeError("pyserial not installed")
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None

    def open(self):
        self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout,
                                 parity=serial.PARITY_EVEN, bytesize=8, stopbits=1)

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    def send(self, data: bytes) -> bool:
        if not self.ser or not self.ser.is_open:
            return False
        self.ser.write(data)
        return True

    def receive(self, max_len: int = 256) -> bytes:
        if not self.ser or not self.ser.is_open:
            return b""
        return self.ser.read(max_len)