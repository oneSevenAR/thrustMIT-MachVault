#!/usr/bin/env python2
# OpenMV Pose Detection with MediaPipe + PyOpenMV + Pygame

import sys
import os
import numpy as np
import pygame
import pyopenmv
import argparse
import time
import cv2
import mediapipe as mp

# <<< GPU Optimization: Avoid CUDA conflicts >>>
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2' 

# <<< MediaPipe Pose Model >>>
mp_pose = mp.solutions.pose  
pose = mp_pose.Pose()  
mp_drawing = mp.solutions.drawing_utils  
mp_drawing_styles = mp.solutions.drawing_styles  

test_script = """
import sensor, image, time
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 300)
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
        except Exception:
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

    fps_values = []

    try:
        while True:
            w, h, data, size, text, fmt = pyopenmv.read_state()
            fps = fps_clock.get_fps()

            if fps > 0:
                fps_values.append(fps)

            mbps = (fps * size) / (1024**2)

            if data is not None:
                # Convert image from Pygame to OpenCV
                image = pygame.image.frombuffer(data.flat[0:], (w, h), 'RGB')
                image = pygame.transform.smoothscale(image, (w * scale, h * scale))
                frame = np.array(pygame.surfarray.array3d(image))  
                frame = np.transpose(frame, (1, 0, 2))  
                frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)  

                # Perform Pose Detection
                results = pose.process(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))  

                if results.pose_landmarks:
                    # Draw pose landmarks with lines
                    mp_drawing.draw_landmarks(
                        frame, 
                        results.pose_landmarks, 
                        mp_pose.POSE_CONNECTIONS,  # Draws the skeleton connections
                        landmark_drawing_spec=mp_drawing_styles.get_default_pose_landmarks_style()
                    )

                # Convert back to Pygame format
                frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)  
                image = pygame.surfarray.make_surface(np.transpose(frame, (1, 0, 2)))  

                if screen is None:
                    screen = pygame.display.set_mode((w * scale, h * scale), pygame.DOUBLEBUF, 32)

                screen.blit(image, (0, 0))
                screen.blit(font.render(f"{fps:.2f} FPS | {mbps:.2f} MB/s", 5, (255, 0, 0)), (0, 0))

                pygame.display.flip()
                fps_clock.tick(30)  # Set a max frame rate to avoid overload

            for event in pygame.event.get():
                if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                    raise KeyboardInterrupt

            clock.tick(1000 // poll_rate)

            # Delay if FPS is higher than expected to smooth out the video
            if fps > 30:
                pygame.time.delay(10)

    except KeyboardInterrupt:
        pass

    pygame.quit()
    pyopenmv.stop_script()

    # Calculate and save average FPS
    if fps_values:
        avg_fps = sum(fps_values) / len(fps_values)
        with open("fps_log.txt", "a") as log_file:
            log_file.write(f"QVGA - {avg_fps:.2f}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="pyopenmv module")
    parser.add_argument("--port", default="/dev/ttyACM0", help="OpenMV camera port (default /dev/ttyACM0)")
    parser.add_argument("--poll", type=int, default=4, help="Poll rate (default 4ms)")
    parser.add_argument("--scale", type=int, default=4, help="Set frame scaling factor (default 4x)")
    args = parser.parse_args()

    pygame_test(args.port, args.poll, args.scale)
