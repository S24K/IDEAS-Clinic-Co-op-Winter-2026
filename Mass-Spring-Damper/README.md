# Mass-Spring Oscillator Measurement and Serial Plotting

This project measures the motion of a forced mass-spring system using:

- an **HC-SR04 ultrasonic sensor** for mass displacement
- a **rotary encoder** for input/arm motion
- an **Arduino** for real-time data acquisition
- a **Python serial plotter** for live visualization

## Project Overview

The system tracks two main motions:

1. **Mass motion**
   - measured using the ultrasonic sensor
   - zeroed at startup using the initial measured distance
   - reported as:
     - raw distance
     - displacement
     - filtered displacement

2. **Input / arm motion**
   - estimated from the rotary encoder angle
   - modeled as a sinusoidal displacement

The Arduino sends the data to the PC over serial, and the Python script plots it live.

---

## Files

### Arduino
`arduino/mass_spring.ino`

Reads:
- HC-SR04 ultrasonic distance
- rotary encoder position
- potentiometer value for motor speed control

Outputs serial data in the format:

time rawDist dispDist filteredDist armHeight

### Python
`python/serial_plotter.py`

Reads the Arduino serial stream and plots:
- Raw Distance
- Displacement
- Filtered Displacement
- Arm Height

---

## Hardware Used

- Arduino
- HC-SR04 ultrasonic sensor
- Rotary encoder
- Adafruit DRV8871 motor driver
- Potentiometer
- Motor-driven eccentric cam / arm mechanism
- Mass-spring setup

---

## Arduino Signal Definitions

### `rawDist`
Absolute distance measured by the ultrasonic sensor in cm.

### `dispDist`
Mass displacement relative to the startup reference:

dispDist = ultrasonicSign * (rawDist - initDist) * ultrasonicScale

### `filteredDist`
Low-pass filtered displacement used to reduce ultrasonic noise.

### `armHeight`
Input displacement estimated from encoder angle:

armHeight = 0.1 * sin(angle)

---

## Serial Output Format

The Arduino prints 5 values separated by spaces:

```text
time rawDist dispDist filteredDist armHeight
