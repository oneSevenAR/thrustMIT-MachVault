import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib.ticker as ticker

# ===== CHANGE THIS to your Arduino port =====
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)  

raw_data = []
kalman_data = []
N = 200  # number of points shown

plt.style.use('ggplot')

fig, ax = plt.subplots()
line1, = ax.plot([], [], label="Raw(G)", color="red")
line2, = ax.plot([], [], label="Kalman(G)", color="blue")
ax.legend()

ax.set_xlim(0, N)
ax.set_xlabel("Samples")
ax.set_ylabel("Magnetic Field (G)")
ax.set_title("Live Hall Sensor Data")

ax.grid(True, which='major', linestyle='--', linewidth=0.5)

def animate(i):
    while ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if ',' not in line:
            return
        try:
            raw, kalman = map(float, line.split(','))
            raw_data.append(raw)
            kalman_data.append(kalman)

            if len(raw_data) > N:
                raw_data.pop(0)
                kalman_data.pop(0)

            line1.set_data(range(len(raw_data)), raw_data)
            line2.set_data(range(len(kalman_data)), kalman_data)

            # Dynamic Y-axis zoom
            all_data = raw_data + kalman_data
            if all_data:
                margin = 5  # zoom padding
                ymin = min(all_data) - margin
                ymax = max(all_data) + margin
                ax.set_ylim(ymin, ymax)
                ax.relim()              # recompute limits
                ax.autoscale_view()    # apply scaling

        except Exception as e:
            print(f"Parse error: {e}")
            pass

ani = FuncAnimation(fig, animate, interval=10)
plt.show()
