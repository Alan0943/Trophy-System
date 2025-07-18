
from gpiozero import Servo
from gpiozero.pins.pigpio import PiGPIOFactory
from time import sleep

factory = PiGPIOFactory()
servo1 = Servo(18, pin_factory=factory)
servo2 = Servo(17, pin_factory=factory)

def move_servo(from_pos, to_pos, step=0.05, delay=0.02):
    if from_pos < to_pos:
        pos = from_pos
        while pos <= to_pos:
            servo1.value = pos
            servo2.value = pos
            sleep(delay)
            pos += step
    else:
        pos = from_pos
        while pos >= to_pos:
            servo1.value = pos
            servo2.value = pos
            sleep(delay)
            pos -= step

try:
    while True:
        print("Moving from 0° to 180°")
        move_servo(-1, 1, step=0.02, delay=0.003)  # slower move
        sleep(1)
        print("Moving from 180° to 0°")
        move_servo(1, -1, step=0.02, delay=0.003)
        sleep(1)

except KeyboardInterrupt:
    servo.value = None
    print("Servo control stopped.")
