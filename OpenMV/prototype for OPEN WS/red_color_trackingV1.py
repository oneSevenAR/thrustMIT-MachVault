import sensor, image, time
from pyb import Servo

EXPOSURE_MICROSECONDS = 1000

# wider LAB thresholds for red detection
# tweak these after testing in ur lighting
RED_THRESHOLDS = [(20, 80, 30, 127, 20, 127)]

SEARCHING_RESOLUTION = sensor.VGA
TRACKING_RESOLUTION = sensor.VGA

SEARCHING_AREA_THRESHOLD = 50
SEARCHING_PIXEL_THRESHOLD = SEARCHING_AREA_THRESHOLD

TRACKING_AREA_THRESHOLD = 200
TRACKING_PIXEL_THRESHOLD = TRACKING_AREA_THRESHOLD

CENTER = 20
RANGE = 60
MIN_POS = CENTER - RANGE
MAX_POS = CENTER + RANGE

DEADBAND = 10
MOVE_MULTIPLIER = 0.01  # faster reaction

SWEEP_DELAY = 30
NO_BLOB_TIMEOUT = 2000

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(SEARCHING_RESOLUTION)
sensor.skip_frames(time=1000)

sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)
# sensor.set_auto_exposure(False, exposure_us=EXPOSURE_MICROSECONDS)
sensor.skip_frames(time=1000)

clock = time.clock()
servo = Servo(1)
servo_pos = CENTER
servo.angle(servo_pos)

last_blob_time = time.ticks_ms()
sweep_dir = 1

ema_cx = None

def clamp(val, mn, mx):
    return max(mn, min(mx, val))

def update_servo(cx):
    global servo_pos, ema_cx
    if ema_cx is None:
        ema_cx = cx
    else:
        alpha = 0.4
        ema_cx = alpha * cx + (1 - alpha) * ema_cx

    error = ema_cx - (640 / 2)

    if abs(error) > DEADBAND:
        move_amt = error * MOVE_MULTIPLIER
        servo_pos -= move_amt
        servo_pos = clamp(servo_pos, MIN_POS, MAX_POS)
        servo.angle(int(servo_pos))

while True:
    clock.tick()
    img = sensor.snapshot()
    blobs = img.find_blobs(RED_THRESHOLDS,
                           area_threshold=SEARCHING_AREA_THRESHOLD,
                           pixels_threshold=SEARCHING_PIXEL_THRESHOLD,
                           merge=True)

    if blobs:
        last_blob_time = time.ticks_ms()
        largest_blob = max(blobs, key=lambda b: b.pixels())
        img.draw_rectangle(largest_blob.rect())
        img.draw_cross(largest_blob.cx(), largest_blob.cy())
        update_servo(largest_blob.cx())
    else:
        if time.ticks_diff(time.ticks_ms(), last_blob_time) > NO_BLOB_TIMEOUT:
            servo_pos += sweep_dir * 3  # lil faster
            if servo_pos > MAX_POS or servo_pos < MIN_POS:
                sweep_dir *= -1
            servo.angle(int(servo_pos))
            time.sleep_ms(SWEEP_DELAY)
