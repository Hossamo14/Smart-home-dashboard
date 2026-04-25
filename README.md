# Smart Home Dashboard

A local ESP32-based smart home dashboard for monitoring environmental sensors and controlling smart home devices through a web interface hosted directly on the ESP32.

The project combines embedded systems, IoT concepts, sensor monitoring, actuator control, WebSocket real-time communication, and a browser-based dashboard served from the ESP32 filesystem.

## Project Overview

This project uses an ESP32 as a local web server to host and control a smart home dashboard. The frontend files, including HTML, CSS, JavaScript, and audio alert files, are stored inside the ESP32 LittleFS filesystem.

The dashboard communicates with the ESP32 using WebSocket for real-time updates and interaction. This allows sensor readings, device states, and alerts to be updated instantly without constantly refreshing the page.

When the ESP32 connects to the local Wi-Fi network, it serves the dashboard through its IP address. Any device connected to the same network can open the dashboard in a browser without needing an external cloud server.

## Features

- ESP32-based local web server
- Real-time communication using WebSocket
- Web dashboard served directly from the ESP32
- LittleFS filesystem for hosting frontend files
- HTML, CSS, and JavaScript dashboard interface
- Sensor monitoring and live status updates
- Smart home device control
- Environmental alerts
- Audio alert system for different sensor conditions
- Local network access without cloud dependency
- PlatformIO project structure

## Tools and Technologies

- ESP32
- PlatformIO
- Arduino Framework
- ESPAsyncWebServer
- AsyncWebSocket
- LittleFS
- HTML
- CSS
- JavaScript
- Sensors and actuators
- Local IoT dashboard architecture

## Project Structure

```text
Smart-home-dashboard/
│
├── data/
│   ├── index.html
│   ├── styles.css
│   ├── app.js
│   ├── test.txt
│   ├── Alert-sound.mp3
│   ├── cold_weather.mp3
│   ├── dry_air.mp3
│   ├── fine_weather.mp3
│   ├── gas_detected.mp3
│   ├── high_humidity.mp3
│   ├── hot_weather.mp3
│   └── rain_detected.mp3
│
├── src/
│   └── main.cpp
│
├── include/
│   └── README
│
├── lib/
│   └── README
│
├── test/
│   └── README
│
├── platformio.ini
├── .gitignore
├── Sensors-Location.pdf
└── README.md
