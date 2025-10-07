import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

ser = serial.Serial('/dev/cu.usbmodem101', 115200, timeout=1)

N = 200  # Number of points on graph
B1_raw_data, B1_kalman_data = [], []
B2_raw_data, B2_kalman_data = [], []

plt.style.use('ggplot')
fig, ax = plt.subplots()
line1, = ax.plot([], [], label="B1 Kalman", color='blue')
line2, = ax.plot([], [], label="B2 Kalman", color='green')
line_net, = ax.plot([], [], label="Net Field", color='red')
ax.set_xlim(0, N)
ax.set_ylim(-2000, 2000)
ax.set_xlabel("Samples")
ax.set_ylabel("Magnetic Field (G)")
ax.legend()
ax.grid(True, linestyle='--', linewidth=0.5)

def animate(i):
    while ser.in_waiting:
        line = ser.readline().decode(errors='ignore').strip()
        if ',' not in line: return
        try:
            B1_raw, B1_kalman, B2_raw, B2_kalman = map(float, line.split(','))
            B1_raw_data.append(B1_raw)
            B1_kalman_data.append(B1_kalman)
            B2_raw_data.append(B2_raw)
            B2_kalman_data.append(B2_kalman)

            if len(B1_kalman_data) > N:
                B1_raw_data.pop(0)
                B1_kalman_data.pop(0)
                B2_raw_data.pop(0)
                B2_kalman_data.pop(0)

            # Net magnetic field
            net_field = [B1 + B2 for B1, B2 in zip(B1_kalman_data, B2_kalman_data)]

            line1.set_data(range(len(B1_kalman_data)), B1_kalman_data)
            line2.set_data(range(len(B2_kalman_data)), B2_kalman_data)
            line_net.set_data(range(len(net_field)), net_field)

        except:
            pass

ani = FuncAnimation(fig, animate, interval=50)
plt.show()