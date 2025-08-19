import sensor, image, time
from pyb import Servo

EXPOSURE_MICROSECONDS = 1000
TRACKING_THRESHOLDS = [(128, 255)]

SEARCHING_RESOLUTION = sensor.VGA
TRACKING_RESOLUTION = sensor.VGA

SEARCHING_AREA_THRESHOLD = 16
SEARCHING_PIXEL_THRESHOLD = SEARCHING_AREA_THRESHOLD

TRACKING_AREA_THRESHOLD = 16
TRACKING_PIXEL_THRESHOLD = TRACKING_AREA_THRESHOLD

TRACKING_EDGE_TOLERANCE = 0.005

CENTER = 20
RANGE = 60
MIN_POS = CENTER - RANGE
MAX_POS = CENTER + RANGE

AVG_FRAMES = 5
DEADBAND = 5

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
        move_amount = error * 0.05
        servo_pos -= move_amount  # invert if it moves wrong
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

        # center on blob with QVGA resolution and ROI
        sensor.set_framesize(TRACKING_RESOLUTION)
        cx = most_dense_blob.cx()
        cy = most_dense_blob.cy()
        x = int(cx - (sensor.width() / 2))
        y = int(cy - (sensor.height() / 2))
        sensor.ioctl(sensor.IOCTL_SET_READOUT_WINDOW, (x, y, sensor.width(), sensor.height()))

        # track while we find blobs
        while True:
            clock.tick()
            img = sensor.snapshot()
            blobs = img.find_blobs(TRACKING_THRESHOLDS,
                                   area_threshold=TRACKING_AREA_THRESHOLD,
                                   pixels_threshold=TRACKING_PIXEL_THRESHOLD)
            if not blobs:
                # reset to search mode
                sensor.set_framesize(SEARCHING_RESOLUTION)
                sensor.ioctl(sensor.IOCTL_SET_READOUT_WINDOW, (0, 0, sensor.width(), sensor.height()))
                cx_history = []  # clear avg on lost target
                break

            most_dense_blob = max(blobs, key=lambda b: b.density())
            img.draw_rectangle(most_dense_blob.rect())
            update_servo(most_dense_blob.cx())

            x_diff = most_dense_blob.cx() - (sensor.width() / 2)
            y_diff = most_dense_blob.cy() - (sensor.height() / 2)

            w_thresh = (sensor.width() / 2) * TRACKING_EDGE_TOLERANCE
            h_thresh = (sensor.height() / 2) * TRACKING_EDGE_TOLERANCE

            if abs(x_diff) > w_thresh or abs(y_diff) > h_thresh:
                x = int(most_dense_blob.cx() - (sensor.width() / 2))
                y = int(most_dense_blob.cy() - (sensor.height() / 2))
                sensor.ioctl(sensor.IOCTL_SET_READOUT_WINDOW, (x, y, sensor.width(), sensor.height()))
    else:
        # sweep servo slowly when no blob
        if time.ticks_diff(time.ticks_ms(), last_blob_time) > 3000:
            servo_pos += sweep_dir * 2
            if servo_pos > MAX_POS or servo_pos < MIN_POS:
                sweep_dir *= -1
            servo.angle(int(servo_pos))
            time.sleep_ms(50)
