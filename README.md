# Pico Vibration Logger

Measures vibration via an ADXL345 accelerometer and logs to flash memory.

```pio run --target upload```

To show progress:

```pio device monitor --port /dev/ttyACM0 --baud 115200 --echo --filter log2file```

typing "dump" dumps the logs.


Wiring:

```ADXL345     Raspberry Pi Pico
VIN    -->  3V3 Out
GND    -->  Ground
SDA    -->  GP4 (I2C0 SDA)
SCL    -->  GP5 (I2C0 SCL)
