import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# ===== Adjust your Arduino port =====
ser = serial.Serial('/dev/cu.usbmodem101', 115200, timeout=1)

# Data buffers
raw_data = []
filtered_data = []
largest_data = []
N = 200  # Number of samples to display

plt.style.use('ggplot')
fig, ax = plt.subplots()
line1, = ax.plot([], [], label="Raw (G)", color="red")
line2, = ax.plot([], [], label="Filtered (G)", color="blue")
line3, = ax.plot([], [], label="Largest Contributor (G)", color="green")
ax.legend()
ax.set_xlim(0, N)
ax.set_xlabel("Samples")
ax.set_ylabel("Magnetic Field (G)")
ax.set_title("Hall Sensor Live Data")
ax.grid(True, linestyle='--', linewidth=0.5)

# Optional: display latest largest contributor
largest_text = ax.text(0.02, 0.95, "", transform=ax.transAxes, 
                       fontsize=12, color="green", verticalalignment='top')

def animate(i):
    while ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if ',' not in line:
            continue
        try:
            raw, filtered, largest = map(float, line.split(','))
            raw_data.append(raw)
            filtered_data.append(filtered)
            largest_data.append(largest)

            # Keep buffer length
            if len(raw_data) > N:
                raw_data.pop(0)
                filtered_data.pop(0)
                largest_data.pop(0)

            # Update lines
            line1.set_data(range(len(raw_data)), raw_data)
            line2.set_data(range(len(filtered_data)), filtered_data)
            line3.set_data(range(len(largest_data)), largest_data)

            # Dynamic Y-axis scaling
            all_data = raw_data + filtered_data + largest_data
            if all_data:
                margin = max(5, 0.1 * max(all_data))  # margin = 10% of max
                ymin = min(all_data) - margin
                ymax = max(all_data) + margin
                ax.set_ylim(ymin, ymax)

            # Update latest largest contributor text
            largest_text.set_text(f"Largest Contributor: {largest:.2f} G")

        except Exception as e:
            print(f"Parse error: {e}")

ani = FuncAnimation(fig, animate, interval=50, cache_frame_data=False)
plt.show()