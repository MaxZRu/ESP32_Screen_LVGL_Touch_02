import lcd
import machine

def config():

    tft = lcd.DPI(
        data = (
            machine.Pin(15),
            machine.Pin(7),
            machine.Pin(6),
            machine.Pin(5),
            machine.Pin(4),
            machine.Pin(9),
            machine.Pin(46),
            machine.Pin(3),
            machine.Pin(8),
            machine.Pin(16),
            machine.Pin(1),
            machine.Pin(14),
            machine.Pin(21),
            machine.Pin(47),
            machine.Pin(48),
            machine.Pin(45),
        ),
        hsync = machine.Pin(39),
        vsync = machine.Pin(40),
        de = machine.Pin(41),
        pclk_pin = machine.Pin(42),
#         timings = (1, 46, 210, 1, 23, 22),
        timings = (48, 40, 40, 31, 13, 1),
        backlight = machine.Pin(2),
        pclk = 14000000,
        width = 800,
        height = 480
    )
    tft.reset()
    tft.init()
    tft.backlight_on()
    return tft
