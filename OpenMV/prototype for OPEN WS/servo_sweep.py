from pyb import Servo
import time

CENTER = 20
RANGE = 60

s = Servo(1)

while True:
    for a in range(CENTER - RANGE, CENTER + RANGE + 1, 1):
        s.angle(a)
        time.sleep_ms(20)
    for a in range(CENTER + RANGE, CENTER - RANGE - 1, -1):
        s.angle(a)
        time.sleep_ms(20)
