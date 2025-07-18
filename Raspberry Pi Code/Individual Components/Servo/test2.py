from gpiozero import Servo
from time import sleep

# Use pigpio for accurate PWM and less jitter
servo = Servo(18, min_pulse_width = 1500/1000000, max_pulse_width = 2500/1000000)  # GPIO 18

try:
    while True:
        print("Rotating to 0°")
        servo.value = -1  # Full left (0°)
        sleep(4)

        print("Rotating to 90°")
        servo.value = 0   # Center (90°)
        sleep(4)

        print("Rotating to 180°")
        servo.value = 1   # Full right (180°)
        sleep(4)

except KeyboardInterrupt:
    print("Stopping servo")
    servo.value = None  # Stop sending signal
