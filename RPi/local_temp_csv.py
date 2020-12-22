import board
import busio
import time
import sys
from pathlib import Path
import adafruit_bme280
import adafruit_mcp9808
from luma.core.interface.serial import i2c as i2c_luma
from luma.core.render import canvas
from luma.oled.device import sh1106
from ds18b20 import DS18B20
import Adafruit_DHT
import pandas as pd
import signal
import threading
import argparse
import subprocess

# Constants
WRITE_DELAY = 1800  # in seconds, time to wait between writing to SD card
DELAY_BETWEEN_MEASUREMENTS = 300  # in seconds, time to wait before getting new data
DHT_PIN = 6
DF_COLUMNS = ['Time', 'BME280 Temperature', 'BME 280 Humidity', 'BME 280 Pressure',
              'BME 280 Altitude', 'MCP 9808 Temperature', 'DS18B20 Temperature', 'DHT22 Temperature',
              'DHT 22 Humidity']


class TempRecorder:

    def __init__(self, quietMode=False):
        self.keepRunning = True
        self.accumulated_data = []
        self.start_time = 0
        self.csv_file = None
        self.e = threading.Event()
        self.quietMode = quietMode

    def writeToDisk(self):
        print('*****Writing data to disk****')
        df = pd.DataFrame(self.accumulated_data, columns=DF_COLUMNS)
        df.to_csv(self.csv_file, mode='a', header=False, index=False)
        self.start_time = time.time()
        self.accumulated_data = []

    def exitGracefully(self, signum, frame):
        print(f'Signal {signal.Signals(signum).name} received!')
        self.keepRunning = False
        self.e.set()

    def main(self):
        print('Starting Data Recorder...')
        # Init i2c
        i2c = busio.I2C(board.SCL, board.SDA)
        # init bme280, MCP sensors
        bme280 = adafruit_bme280.Adafruit_BME280_I2C(i2c, address=0x76)
        mcp = adafruit_mcp9808.MCP9808(i2c)

        # location's pressure (hPa) at sea level
        bme280.sea_level_pressure = 1017.1

        # init lcd screen - needs separate i2c library
        i2c_lcd = i2c_luma(port=1, address=0x3C)
        lcd = sh1106(i2c_lcd)

        # initialize dht and ds18b20 sensors
        dhtsensor = Adafruit_DHT.AM2302

        # uses kernel mode driver
        ds18b20 = DS18B20()

        # Wait for correct time
        subprocess.run(["chronyc", "waitsync"], check=True)
        print('Time synchronized!')

        self.csv_file = Path(f'/measurements/recorded_data_{time.strftime("%F_%T")}.csv')

        # write CSV headers to disk
        df = pd.DataFrame(columns=DF_COLUMNS)
        df.to_csv(self.csv_file, index=False)

        # record data
        self.accumulated_data = []
        self.start_time = time.time()

        # try:
        # Signal handler
        signal.signal(signal.SIGINT, self.exitGracefully)
        signal.signal(signal.SIGTERM, self.exitGracefully)

        while self.keepRunning:

            bme280T = bme280.temperature
            bme280H = bme280.relative_humidity
            bme280P = bme280.pressure
            bme280A = bme280.altitude

            mcpT = mcp.temperature

            ds18T = ds18b20.get_temperature()

            DHT_humidity, DHT_temperature = Adafruit_DHT.read_retry(dhtsensor, DHT_PIN)

            if not self.quietMode:
                print('--------------\nBME 280')
                print("Temperature: %0.1f C" % bme280T)
                print("Humidity: %0.1f %%" % bme280H)
                print("Pressure: %0.1f hPa" % bme280P)
                print("Altitude = %0.2f meters\n" % bme280A)

                print("MCP9808 Temperature: {} C\n".format(mcpT))

                print('DS18B20 Temperature: {0:0.3f} °C\n'.format(ds18T))

                print('DHT22')

                if DHT_humidity is not None and DHT_temperature is not None:
                    print(f'Temperature: {DHT_temperature:.1f}')
                    print(f'Humidity: {DHT_humidity:.1f}')
                else:
                    print('DHT22 read failure.')

            with canvas(lcd) as draw:
                # draw.rectangle(lcd.bounding_box, outline="white", fill="black")
                draw.text((0, 0), f'BME T: {bme280T:.1f} °C', fill='white')
                draw.text((0, 9), f'BME H: {bme280H:.1f} %', fill='white')
                draw.text((0, 18), f'BME P: {bme280P:.1f} hPa', fill='white')
                draw.text((0, 27), f'BME A: {bme280A:.2f} m', fill='white')
                draw.text((0, 36), f'MCP T: {mcpT:.1f} °C', fill='white')
                draw.text((0, 45), f'DS18 T: {ds18T:.1f} °C', fill='white')

            self.accumulated_data.append({
                'Time': time.strftime("%F %T"),
                'BME280 Temperature': bme280T,
                'BME 280 Humidity': bme280H,
                'BME 280 Pressure': bme280P,
                'BME 280 Altitude': bme280A,
                'MCP 9808 Temperature': mcpT,
                'DS18B20 Temperature': ds18T,
                'DHT22 Temperature': DHT_temperature,
                'DHT 22 Humidity': DHT_humidity
            })

            if time.time() - self.start_time > WRITE_DELAY:
                self.writeToDisk()

            self.e.clear()
            self.e.wait(timeout=DELAY_BETWEEN_MEASUREMENTS)

        self.writeToDisk()
        print('Bye!')
        sys.exit(0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Record temp and humidity data to a CSV file.')
    parser.add_argument('--quiet', '-q', action='store_true',
                        help='Active quiet mode to suppress printing of data to terminal/log')

    args = parser.parse_args()
    TempRecorder(quietMode=args.quiet).main()
