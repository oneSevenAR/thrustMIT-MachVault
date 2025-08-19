import serial
import time
import sys
import io

# Force UTF-8 for Windows terminal output
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

# Serial port configuration
port = 'COM8'
baud_rate = 115200

# Try opening serial port
try:
    ser = serial.Serial(port, baud_rate, timeout=1)
    print(f"Logging started on {port} at {baud_rate} baud.")
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    sys.exit(1)

# Open CSV file for writing
filename = "log.csv"
with open(filename, "w", encoding="utf-8") as f:
    f.write("Timestamp,Data\n")  # CSV header

    try:
        while True:
            line_bytes = ser.readline()
            try:
                line = line_bytes.decode('utf-8', errors='replace').strip()
            except UnicodeDecodeError:
                continue  # Skip line if completely unreadable

            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            print(f"{timestamp} > {line}")  # Print to console

            f.write(f"{timestamp},{line}\n")
            f.flush()

    except KeyboardInterrupt:
        print("\nLogging stopped by user.")

    finally:
        ser.close()
        print("Serial port closed.")
