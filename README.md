# ATmega328P Climate Control System

An automated environment monitoring and actuation node built on the **ATmega328P** microcontroller. This project demonstrates a hybrid firmware approach combining low-level register concepts with structured C logic for non-blocking sensor acquisition and relay control.

---

## Features

* **Sensor Integration:** Reads real-time temperature and humidity data from a DHT11 sensor.
* **Hysteresis & Threshold Control:** Prevents rapid relay fluttering near setpoints by implementing threshold logic.

---

## Hardware Requirements

* **Microcontroller:** ATmega328P (Bare chip or Arduino Uno board)
* **Environment Sensor:** DHT11 (Temperature & Humidity)
* **Actuator:** 5V Relay Module (Fan / Heating / Humidifier)
* **Status Indicators:** LEDs for system status & active relay state
* **Power Supply:** 5V DC regulated

---

## Architecture Overview 
[ DHT11 Sensor ] ───► [ ATmega328P Node ] ───► [ Relay Module ]
│
(FSM Logic &
Non-blocking Timer)

![Circuit Diagram](circuit_diagram.png)
1. **Data Acquisition:** Reads temperature and humidity at periodic, non-blocking intervals.
2. **Decision Engine:** Compares current metrics against defined threshold limits.
3. **Actuation:** Toggles the relay state to drive environmental control hardware.
