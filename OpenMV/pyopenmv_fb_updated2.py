#!/usr/bin/env python2
# This file is part of the OpenMV project.
#
# Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
# Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
#
# This work is licensed under the MIT license, see the file LICENSE for details.
#
# An example script using pyopenmv to grab the framebuffer.

import sys
import numpy as np
import pygame
import pyopenmv
import argparse
import time

test_script = """
import sensor, image, time
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QCIF)
sensor.skip_frames(time = 2000)
clock = time.clock()

while(True):
    clock.tick()
    img = sensor.snapshot()
    print(clock.fps(), " FPS")
"""

def pygame_test(port, poll_rate, scale):
    pygame.init()
    pyopenmv.disconnect()

    connected = False
    for i in range(10):
        try:
            pyopenmv.init(port, baudrate=921600, timeout=0.050)
            connected = True
            break
        except Exception as e:
            time.sleep(0.100)

    if not connected:
        print("Failed to connect to OpenMV's serial port.")
        sys.exit(1)

    pyopenmv.set_timeout(1*2)
    pyopenmv.stop_script()
    pyopenmv.enable_fb(True)
    pyopenmv.exec_script(test_script)

    screen = None
    clock = pygame.time.Clock()
    fps_clock = pygame.time.Clock()
    font = pygame.font.SysFont("monospace", 30)

    fps_values = []  # List to store FPS values

    try:
        while True:
            w, h, data, size, text, fmt = pyopenmv.read_state()
            fps = fps_clock.get_fps()
            
            if fps > 0:
                fps_values.append(fps)  # Store FPS value

            mbps = (fps * size) / (1024**2)  # Convert to MB/s

            if data is not None:
                image = pygame.image.frombuffer(data.flat[0:], (w, h), 'RGB')
                image = pygame.transform.smoothscale(image, (w * scale, h * scale))

                if screen is None:
                    screen = pygame.display.set_mode((w * scale, h * scale), pygame.DOUBLEBUF, 32)

                screen.blit(image, (0, 0))
                screen.blit(font.render(f"{fps:.2f} FPS | {mbps:.2f} MB/s", 5, (255, 0, 0)), (0, 0))

                pygame.display.flip()
                fps_clock.tick(1000 // poll_rate)

            for event in pygame.event.get():
                if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                    raise KeyboardInterrupt

            clock.tick(1000 // poll_rate)
    except KeyboardInterrupt:
        pass

    pygame.quit()
    pyopenmv.stop_script()

    # Calculate and save average FPS
    if fps_values:
        avg_fps = sum(fps_values) / len(fps_values)
        with open("fps_log.txt", "a") as log_file:
            log_file.write(f"QCIF - {avg_fps:.2f}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="pyopenmv module")
    parser.add_argument("--port", default="/dev/ttyACM0", help="OpenMV camera port (default /dev/ttyACM0)")
    parser.add_argument("--poll", type=int, default=4, help="Poll rate (default 4ms)")
    parser.add_argument("--scale", type=int, default=4, help="Set frame scaling factor (default 4x)")
    args = parser.parse_args()

    pygame_test(args.port, args.poll, args.scale)
