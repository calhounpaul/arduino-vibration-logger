# Pico Vibration Logger

Measures vibration via an ADXL345 accelerometer and logs to flash memory.

```pio run --target upload```

To show progress:

```sudo pio device monitor --port /dev/ttyACM0 --baud 115200 --echo --filter log2file```

typing "dump" dumps the logs.
