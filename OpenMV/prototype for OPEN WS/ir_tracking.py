import sensor, image, time, pyb

# === CONFIG ===
EXPOSURE_MICROSECONDS = 1000
TRACKING_THRESHOLDS = [(128, 255)]  # For bright IR object

SEARCHING_RESOLUTION = sensor.VGA
SEARCHING_AREA_THRESHOLD = 16
SEARCHING_PIXEL_THRESHOLD = SEARCHING_AREA_THRESHOLD

TRACKING_RESOLUTION = sensor.QVGA
TRACKING_AREA_THRESHOLD = 256
TRACKING_PIXEL_THRESHOLD = TRACKING_AREA_THRESHOLD

TRACKING_EDGE_TOLERANCE = 0.05  # blob can drift 5% from center

SERVO_MIN = -70
SERVO_MAX = 70
SEARCH_TIMEOUT = 3000  # ms

# === INIT ===
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(SEARCHING_RESOLUTION)
sensor.skip_frames(time=1000)
clock = time.clock()

sensor.set_auto_gain(False)
sensor.set_auto_exposure(False, exposure_us=EXPOSURE_MICROSECONDS)
sensor.skip_frames(time=1000)

x, y, sensor_w, sensor_h = sensor.ioctl(sensor.IOCTL_GET_READOUT_WINDOW)

# Servos
pan_servo = pyb.Servo(1)
tilt_servo = pyb.Servo(2)
pan_angle = 0
tilt_angle = 0

# === Functions ===
def move_servos(pan, tilt):
    global pan_angle, tilt_angle
    pan_angle = max(SERVO_MIN, min(SERVO_MAX, pan))
    tilt_angle = max(SERVO_MIN, min(SERVO_MAX, tilt))
    pan_servo.angle(pan_angle)
    tilt_servo.angle(tilt_angle)
    print("[DEBUG] Servo move: pan=%d tilt=%d" % (pan_angle, tilt_angle))

def get_mapped_centroid(b):
    x, y, w, h = sensor.ioctl(sensor.IOCTL_GET_READOUT_WINDOW)
    ratio = min(w / float(sensor.width()), h / float(sensor.height()))

    mapped_cx = (b.cx() - (sensor.width() / 2.0)) * ratio
    mapped_cx += (w - (sensor.width() * ratio)) / 2.0
    mapped_cx += x + (sensor_w / 2.0)

    mapped_cy = (b.cy() - (sensor.height() / 2.0)) * ratio
    mapped_cy += (h - (sensor.height() * ratio)) / 2.0
    mapped_cy += y + (sensor_h / 2.0)

    return (mapped_cx, mapped_cy)

# Initial calibration sweep
print("[DEBUG] Calibration sweep start")
for a in range(SERVO_MIN, SERVO_MAX + 1, 10):
    move_servos(a, 0)
    time.sleep_ms(50)
for a in range(SERVO_MAX, SERVO_MIN - 1, -10):
    move_servos(a, 0)
    time.sleep_ms(50)
print("[DEBUG] Calibration sweep done")

last_seen = time.ticks_ms()

# === Main loop ===
while True:
    clock.tick()
    img = sensor.snapshot()

    # Search mode
    blobs = img.find_blobs(TRACKING_THRESHOLDS,
                           area_threshold=SEARCHING_AREA_THRESHOLD,
                           pixels_threshold=SEARCHING_PIXEL_THRESHOLD)

    if blobs:
        most_dense_blob = max(blobs, key=lambda x: x.density())
        img.draw_rectangle(most_dense_blob.rect())
        img.draw_cross(most_dense_blob.cx(), most_dense_blob.cy())
        last_seen = time.ticks_ms()
        print("[DEBUG] Found blob in search mode")

        # Switch to tracking
        sensor.set_framesize(TRACKING_RESOLUTION)

        while True:
            clock.tick()
            img = sensor.snapshot()

            blobs = img.find_blobs(TRACKING_THRESHOLDS,
                                   area_threshold=TRACKING_AREA_THRESHOLD,
                                   pixels_threshold=TRACKING_PIXEL_THRESHOLD)

            if not blobs:
                print("[DEBUG] Lost blob, back to search")
                sensor.set_framesize(SEARCHING_RESOLUTION)
                break

            most_dense_blob = max(blobs, key=lambda x: x.density())
            img.draw_rectangle(most_dense_blob.rect())
            img.draw_cross(most_dense_blob.cx(), most_dense_blob.cy())

            cx_diff = most_dense_blob.cx() - (sensor.width() / 2.0)
            cy_diff = most_dense_blob.cy() - (sensor.height() / 2.0)

            w_threshold = (sensor.width() / 2.0) * TRACKING_EDGE_TOLERANCE
            h_threshold = (sensor.height() / 2.0) * TRACKING_EDGE_TOLERANCE

            if abs(cx_diff) > w_threshold:
                pan_angle -= int(cx_diff / 20)
            if abs(cy_diff) > h_threshold:
                tilt_angle -= int(cy_diff / 20)

            move_servos(pan_angle, tilt_angle)
            print("[DEBUG] Tracking blob: cx=%d cy=%d fps=%.2f" %
                  (most_dense_blob.cx(), most_dense_blob.cy(), clock.fps()))

    else:
        if time.ticks_diff(time.ticks_ms(), last_seen) > SEARCH_TIMEOUT:
            print("[DEBUG] Sweep searching")
            for a in range(SERVO_MIN, SERVO_MAX + 1, 5):
                move_servos(a, 0)
                img = sensor.snapshot()
                blobs = img.find_blobs(TRACKING_THRESHOLDS,
                                       area_threshold=SEARCHING_AREA_THRESHOLD,
                                       pixels_threshold=SEARCHING_PIXEL_THRESHOLD)
                if blobs:
                    break
            last_seen = time.ticks_ms()

    print("[DEBUG] Search mode FPS=%.2f" % clock.fps())
