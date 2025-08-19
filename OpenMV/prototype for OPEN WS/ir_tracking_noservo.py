# 本示例演示了如何使用读取窗口readout控件以非常高的速度读取相机传感器像素阵列的一小部分，并四处移动该读取窗口。

# 本示例是使用OV5640传感器在OpenMV Cam H7 Plus上设计和测试的

import sensor, image, time

EXPOSURE_MICROSECONDS = 1000
TRACKING_THRESHOLDS = [(128, 255)] # 当降低曝光量时，所有物体都会变暗。

SEARCHING_RESOLUTION = sensor.VGA
SEARCHING_AREA_THRESHOLD = 16
SEARCHING_PIXEL_THRESHOLD = SEARCHING_AREA_THRESHOLD

TRACKING_RESOLUTION = sensor.VGA
TRACKING_AREA_THRESHOLD = 16
TRACKING_PIXEL_THRESHOLD = TRACKING_AREA_THRESHOLD

TRACKING_EDGE_TOLERANCE = 0.005 # Blob可以从中心移开5％。

sensor.reset()                         # 复位并初始化传感器。

sensor.set_pixformat(sensor.GRAYSCALE) # Set pixel format to GRAYSCALE
#设置图像色彩格式，有RGB565色彩图和GRAYSCALE灰度图两种

sensor.set_framesize(SEARCHING_RESOLUTION)
sensor.skip_frames(time = 1000)        # 等待设置生效。
clock = time.clock()                   # 创建一个时钟对象来跟踪FPS帧率。

sensor.set_auto_gain(False)            # 关闭自动增益，因为它会振荡。
sensor.set_auto_exposure(False, exposure_us=EXPOSURE_MICROSECONDS)
sensor.skip_frames(time = 1000)

# sensor_w和sensor_h是图像传感器的原始像素w/h (x/y初始值为0)。
x, y, sensor_w, sensor_h = sensor.ioctl(sensor.IOCTL_GET_READOUT_WINDOW)

while(True):
    clock.tick()
    img = sensor.snapshot()

    # 我们需要找到一个要跟踪的IR红外对象 - 它可能非常亮。
    blobs = img.find_blobs(TRACKING_THRESHOLDS,
                           area_threshold=SEARCHING_AREA_THRESHOLD,
                           pixels_threshold=SEARCHING_PIXEL_THRESHOLD)

    if len(blobs):
        most_dense_blob = max(blobs, key = lambda x: x.density())
        img.draw_rectangle(most_dense_blob.rect())

        def get_mapped_centroid(b):
            # 默认情况下，读出窗口readout的整个传感器像素阵列设置为x/y == 0。
            # 您所看到的分辨率(如果通过从相机的读取窗口中获取像素来获得),
            # x/y位置相对于传感器中心。
            x, y, w, h = sensor.ioctl(sensor.IOCTL_GET_READOUT_WINDOW)

            # 相机驱动程序将尝试缩放以适合您传递的任何分辨率，
            # 以达到适合传感器的最大宽度/高度，同时保持宽高比。
            ratio = min(w / float(sensor.width()), h / float(sensor.height()))

            # 引用cx()到视口的中心，然后缩放到读数。
            mapped_cx = (b.cx() - (sensor.width() / 2.0)) * ratio
            # 因为我们要保持宽高比，所以x可能会有偏移。
            mapped_cx += (w - (sensor.width() * ratio)) / 2.0
            # 加上我们从传感器中心的位移
            mapped_cx += x + (sensor_w / 2.0)

            # 引用cy()到视口的中心，然后缩放到读数。
            mapped_cy = (b.cy() - (sensor.height() / 2.0)) * ratio
            # 因为我们要保持宽高比，因此y可能会有偏移。
            mapped_cy += (h - (sensor.height() * ratio)) / 2.0
            # 加上我们从传感器中心的位移
            mapped_cy += y + (sensor_h / 2.0)

            return (mapped_cx, mapped_cy) # 传感器阵列上的X/Y位置。

        def center_on_blob(b, res):
            mapped_cx, mapped_cy = get_mapped_centroid(b)

            # 切换到res（如果res不变，则不执行任何操作）。
            sensor.set_framesize(res)

            # 构造读出窗口。x/y是距中心的偏移量。
            x = int(mapped_cx - (sensor_w / 2.0))
            y = int(mapped_cy - (sensor_h / 2.0))
            w = sensor.width()
            h = sensor.height()

            # Focus on the centroid.
            # 关注中心。
            sensor.ioctl(sensor.IOCTL_SET_READOUT_WINDOW, (x, y, w, h))

            # 看看我们是否处于边缘。
            new_x, new_y, w, h = sensor.ioctl(sensor.IOCTL_GET_READOUT_WINDOW)

            # 如果需要，可以使用这些误差值来驱动伺服器移动摄像头。
            x_error = x - new_x
            y_error = y - new_y

            if x_error < 0: print("-X Limit Reached ", end="")
            if x_error > 0: print("+X Limit Reached ", end="")
            if y_error < 0: print("-Y Limit Reached ", end="")
            if y_error > 0: print("+Y Limit Reached ", end="")

        center_on_blob(most_dense_blob, TRACKING_RESOLUTION)

        # 这个循环将以更高的readout读取速度和更低的分辨率跟踪色块。
        while(True):
            clock.tick()
            img = sensor.snapshot()

            # 在低分辨率图像中找到色块。
            blobs = img.find_blobs(TRACKING_THRESHOLDS,
                                   area_threshold=TRACKING_AREA_THRESHOLD,
                                   pixels_threshold=TRACKING_PIXEL_THRESHOLD)

            # 如果我们没有找到这个色块，那么我们需要找到一个新的。
            if not len(blobs):
                # 重置分辨率。
                sensor.set_framesize(SEARCHING_RESOLUTION)
                sensor.ioctl(sensor.IOCTL_SET_READOUT_WINDOW, (sensor_w, sensor_h))
                break

            # 缩小色块列表并突出显示这个色块。
            most_dense_blob = max(blobs, key = lambda x: x.density())
            img.draw_rectangle(most_dense_blob.rect())

            print(clock.fps(), "BLOB cx:%d, cy:%d" % get_mapped_centroid(most_dense_blob))

            x_diff = most_dense_blob.cx() - (sensor.width() / 2.0)
            y_diff = most_dense_blob.cy() - (sensor.height() / 2.0)

            w_threshold = (sensor.width() / 2.0) * TRACKING_EDGE_TOLERANCE
            h_threshold = (sensor.height() / 2.0) * TRACKING_EDGE_TOLERANCE

            # 如果色块开始移出视野，请重新将其居中（会消耗FPS帧率）。
            if abs(x_diff) > w_threshold or abs(y_diff) > h_threshold:
                center_on_blob(most_dense_blob, TRACKING_RESOLUTION)

    print(clock.fps())
