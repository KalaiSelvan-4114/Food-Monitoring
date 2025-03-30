import serial
import pymongo
import time
import sys
import serial.tools.list_ports

def find_arduino_port():
    """Find the Arduino port automatically"""
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        if "Arduino" in port.description or "CH340" in port.description:
            return port.device
    return None

def setup_arduino():
    try:
        # Find Arduino port
        port = find_arduino_port()
        if not port:
            print("Error: Could not find Arduino. Please check:")
            print("1. Is the Arduino connected?")
            print("2. Is the correct driver installed?")
            print("\nAvailable ports:")
            for port in serial.tools.list_ports.comports():
                print(f"- {port.device}: {port.description}")
            sys.exit(1)

        print(f"Found Arduino on {port}")
        
        # Try to close any existing connections first
        try:
            arduino = serial.Serial(port, 9600)
            arduino.close()
            time.sleep(1)  # Wait for port to be fully released
        except:
            pass
            
        # Now open a new connection
        arduino = serial.Serial(port, 9600, timeout=1)
        time.sleep(2)  # Wait for Arduino to initialize
        return arduino
    except serial.SerialException as e:
        print(f"Error: Could not open {port}. Please check:")
        print("1. Is another program using the port?")
        print("2. Do you have permission to access the port?")
        print("3. Is the Arduino properly connected?")
        print(f"Detailed error: {e}")
        sys.exit(1)

# Connect to Arduino
try:
    arduino = setup_arduino()
except Exception as e:
    print(f"Failed to setup Arduino: {e}")
    sys.exit(1)

# Connect to MongoDB
try:
    client = pymongo.MongoClient("mongodb://localhost:27017/")
    db = client["food_monitoring"]
    collection = db["sensor_data"]
    print("Successfully connected to MongoDB")
except Exception as e:
    print(f"Failed to connect to MongoDB: {e}")
    arduino.close()
    sys.exit(1)

print("Logging data to MongoDB...")

try:
    while True:
        try:
            # Read data from Arduino
            if arduino.in_waiting:
                raw_data = arduino.readline().decode().strip()
                values = raw_data.split(",")  # Split values by comma

                # Ensure correct data format
                if len(values) != 4:
                    print(f"Invalid data format received: {raw_data}")
                    continue

                temperature = float(values[0])
                humidity = float(values[1])
                methaneLevel = int(values[2])
                status = values[3]

                # Create data object
                data = {
                    "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
                    "temperature": temperature,
                    "humidity": humidity,
                    "methaneLevel": methaneLevel,
                    "status": status
                }

                # Insert into MongoDB
                collection.insert_one(data)
                print(f"Saved: {data}")

        except serial.SerialException as e:
            print(f"Serial communication error: {e}")
            print("Attempting to reconnect...")
            arduino.close()
            time.sleep(1)
            arduino = setup_arduino()
            continue
        except Exception as e:
            print(f"Error reading data: {e}")
            time.sleep(5)  # Wait before retrying
            continue

        time.sleep(5)  # Wait 5 seconds before the next reading

except KeyboardInterrupt:
    print("\nStopping data collection...")
finally:
    # Clean up
    try:
        arduino.close()
        print("Arduino connection closed")
    except:
        pass
    try:
        client.close()
        print("MongoDB connection closed")
    except:
        pass 