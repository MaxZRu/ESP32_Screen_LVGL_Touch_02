import time
from collections import namedtuple
try:
    import lvgl as lv
except ImportError:
    pass

GT911_CONFIG_START = const(0x8047)
GT911_X_OUTPUT_MAX_LOW = const(0x8048)
GT911_X_OUTPUT_MAX_HIGH = const(0x8049)
GT911_Y_OUTPUT_MAX_LOW = const(0x804A)
GT911_Y_OUTPUT_MAX_HIGH = const(0x804B)
GT911_CONFIG_CHKSUM = const(0x80FF)
GT911_CONFIG_FRESH = const(0X8100)

GT911_POINT_INFO = const(0X814E)
GT911_POINT_1 = const(0X814F)
GT911_POINT_2 = const(0X8157)
GT911_POINT_3 = const(0X815F)
GT911_POINT_4 = const(0X8167)
GT911_POINT_5 = const(0X816F)

_GT911_ADDR1 = const(0x5D)
_GT911_ADDR2 = const(0x14)


class TouchPoint:
    id = 0
    x = 0
    y = 0
    size = 0


class GT911:
    ROTATION_LEFT     = const(0)
    ROTATION_INVERTED = const(1)
    ROTATION_RIGHT    = const(2)
    ROTATION_NORMAL   = const(3)

    def __init__(self, i2c, addr=_GT911_ADDR1, intr=None, reset=None, width=-1, height=-1) -> None:
        self._i2c =i2c
        self._addr = addr
        self._intr = intr
        self._reset = reset
        self._width = width
        self._height = height
        self._rotation = self.ROTATION_NORMAL
        self.is_touched = False
        self.points = [ TouchPoint() for _ in range(5) ]

        self._config_buf = bytearray(0xFF - 0x46)
        self.reset()

    def reset(self) -> None:
        if self._intr and self._reset:
            self._intr(0)
            self._reset(0)
            time.sleep_ms(10)
            self._intr(int(self._addr == _GT911_ADDR2))
            time.sleep_ms(1)
            self._reset(1)
            time.sleep_ms(5)
            self._intr(0)
            time.sleep_ms(50)
            self._intr.init(self._intr.IN)
            time.sleep_ms(50)
        self._read_bytes(GT911_CONFIG_START, self._config_buf)

    def set_rotation(self, rotation) -> None:
        self._rotation = rotation

    def set_resolution(self, width, height):
        self._width = width
        self._height = height
        self._config_buf[GT911_X_OUTPUT_MAX_LOW - GT911_CONFIG_START] = width & 0xFF
        self._config_buf[GT911_X_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = (width > 8) & 0xFF
        self._config_buf[GT911_Y_OUTPUT_MAX_LOW - GT911_CONFIG_START] = height & 0xFF
        self._config_buf[GT911_Y_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = (height > 8) & 0xFF

    def get_gesture(self) -> int:
        pass

    def read(self):
        data = bytearray(7)
        id = 0
        x = 0
        y = 0
        size = 0

        point_info = self._read_byte(GT911_POINT_INFO)
        buffer_status = point_info >> 7 & 1
        proximity_valid = point_info >> 5 & 1
        have_key = point_info >> 4 & 1
        is_large_detect = point_info >> 6 & 1
        touches = point_info & 0xF
        self.is_touched = touches > 0
        if buffer_status == 1 and self.is_touched:
            for i in range(touches):
                self._read_bytes(GT911_POINT_1 + i * 8, data)
                self.points[i].id, self.points[i].x, self.points[i].y, self.points[i].size = self.read_point(data)
        self._write_byte(GT911_POINT_INFO, 0)

    def read_point(self, data):
        temp = 0
        id = data[0]
        x = data[1] + (data[2] << 8)
        y = data[3] + (data[4] << 8)
        size = data[5] + (data[6] << 8)
        if self._rotation == self.ROTATION_NORMAL:
            x = self._width - x
            y = self._height - y
        elif self._rotation == self.ROTATION_LEFT:
            temp = x
            x = self._width - y
            y = temp
        elif self._rotation == self.ROTATION_INVERTED:
            x = x
            y = y
        elif self._rotation == self.ROTATION_RIGHT:
            temp = x
            x = y
            y = self._height - temp
        return id, x, y, size

    def _read_bytes(self, reg, buf):
        reg_buf = bytearray(2)
        reg_buf[0] = (reg >> 8 & 0xFF)
        reg_buf[1] = (reg & 0xFF)
        self._i2c.writeto(self._addr, reg_buf)
        self._i2c.readfrom_into(self._addr, buf)

    def _write_bytes(self):
        pass

    def _read_byte(self, reg):
        reg_buf = bytearray(2)
        reg_buf[0] = (reg >> 8 & 0xFF)
        reg_buf[1] = (reg & 0xFF)
        self._i2c.writeto(self._addr, reg_buf)
        buf = bytearray(1)
        self._i2c.readfrom_into(self._addr, buf)
        return buf[0]

    def _write_byte(self, reg, val):
        buf = bytearray(3)
        buf[0] = (reg >> 8 & 0xFF)
        buf[1] = (reg & 0xFF)
        buf[2] = val & 0xFF
        self._i2c.writeto(self._addr, buf)

    def _calculate_checksum(self):
        checksum = 0
        for i in self._config_buf:
            checksum += i
        checksum = (~checksum) + 1
        self._config_buf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START] = checksum;

    def _reflash_config(self):
        self._calculate_checksum()
        self._write_byte(GT911_CONFIG_CHKSUM, self._config_buf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START])
        self._write_byte(GT911_CONFIG_FRESH, 1)

    def lvgl_read(self, indev_drv, data):
        self.read()
        if self.is_touched:
            data.point.x = self.points[0].x
            data.point.y = self.points[0].y
            data.state = lv.INDEV_STATE.PRESSED
            return False
        data.state = lv.INDEV_STATE.RELEASED
        return False
