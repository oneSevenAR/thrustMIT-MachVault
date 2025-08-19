import sensor, image, time
from pyb import Servo

# ---- Config ----
EXPOSURE_MICROSECONDS = 1000
TRACKING_THRESHOLDS = [(128, 255)]

SEARCHING_RESOLUTION = sensor.VGA
TRACKING_RESOLUTION = sensor.VGA  # Both searching and tracking VGA

SEARCHING_AREA_THRESHOLD = 50  # bump thresholds to ignore small noise
SEARCHING_PIXEL_THRESHOLD = SEARCHING_AREA_THRESHOLD

TRACKING_AREA_THRESHOLD = 200
TRACKING_PIXEL_THRESHOLD = TRACKING_AREA_THRESHOLD

TRACKING_EDGE_TOLERANCE = 0.1  # let blob wander a bit more near edges before recentering

CENTER = 20
RANGE = 60
MIN_POS = CENTER - RANGE
MAX_POS = CENTER + RANGE

AVG_FRAMES = 10  # more frames for smoother avg
DEADBAND = 10    # bigger deadzone to avoid jitter

MOVE_MULTIPLIER = 0.01  # slow servo movement to prevent overshoot

SWEEP_DELAY = 50
NO_BLOB_TIMEOUT = 3000

# ---- Init ----
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(SEARCHING_RESOLUTION)
sensor.skip_frames(time=1000)

sensor.set_auto_gain(False)
sensor.set_auto_exposure(False, exposure_us=EXPOSURE_MICROSECONDS)
sensor.skip_frames(time=1000)

clock = time.clock()
servo = Servo(1)
servo_pos = CENTER
servo.angle(servo_pos)

last_blob_time = time.ticks_ms()
sweep_dir = 1
cx_history = []

def clamp(value, minimum, maximum):
    return max(minimum, min(maximum, value))

def update_servo(cx):
    global servo_pos, cx_history
    cx_history.append(cx)
    if len(cx_history) > AVG_FRAMES:
        cx_history.pop(0)
    avg_cx = sum(cx_history) / len(cx_history)

    error = avg_cx - (640 / 2)  # fixed center for VGA

    if abs(error) > DEADBAND:
        move_amount = error * MOVE_MULTIPLIER
        servo_pos -= move_amount  # invert if needed, test it
        servo_pos = clamp(servo_pos, MIN_POS, MAX_POS)
        servo.angle(int(servo_pos))

while True:
    clock.tick()
    img = sensor.snapshot()
    blobs = img.find_blobs(TRACKING_THRESHOLDS,
                           area_threshold=SEARCHING_AREA_THRESHOLD,
                           pixels_threshold=SEARCHING_PIXEL_THRESHOLD)

    if blobs:
        last_blob_time = time.ticks_ms()
        most_dense_blob = max(blobs, key=lambda b: b.density())
        img.draw_rectangle(most_dense_blob.rect())

        # fixed VGA, no res changes, no ROI cropping
        update_servo(most_dense_blob.cx())

        x_diff = most_dense_blob.cx() - (sensor.width() / 2)
        y_diff = most_dense_blob.cy() - (sensor.height() / 2)

        w_thresh = (sensor.width() / 2) * TRACKING_EDGE_TOLERANCE
        h_thresh = (sensor.height() / 2) * TRACKING_EDGE_TOLERANCE

        # no ROI recentering since fixed res, but can add if needed later

    else:
        # sweep servo slowly when no blob
        if time.ticks_diff(time.ticks_ms(), last_blob_time) > NO_BLOB_TIMEOUT:
            servo_pos += sweep_dir * 2
            if servo_pos > MAX_POS or servo_pos < MIN_POS:
                sweep_dir *= -1
            servo.angle(int(servo_pos))
            time.sleep_ms(SWEEP_DELAY)
