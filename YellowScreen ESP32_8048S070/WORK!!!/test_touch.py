import gt911
from machine import Pin, I2C

i2c = I2C(1, scl=Pin(20), sda=Pin(19), freq=400000)

tp = gt911.GT911(i2c, width=800, height=480)
tp.set_rotation(tp.ROTATION_NORMAL)

while True:
    tp.read()
    if tp.is_touched:
        for point in tp.points:
          print("Touch ")
          print("  id:", point.id)
          print("  x:", point.x)
          print("  y:", point.y)
          print("  size:", point.size)

