# &#x20;ArduinoP4Camera

# 

# Ein Arduino-Projekt zur Ansteuerung einer \*\*OV5647-Kamera\*\* über die \*\*MIPI-CSI-Schnittstelle\*\* mit einem \*\*ESP32-P4C6\*\*.

# 

# Die aufgenommenen Kamerabilder werden als \*\*MJPEG-Stream über HTTP\*\* übertragen und können direkt im Webbrowser angezeigt werden. Der Webserver läuft dabei direkt auf dem ESP32-P4C6, sodass kein zusätzlicher Server benötigt wird.

# 

# &#x20;Features

# 

# \- \*\*Mikrocontroller:\*\* ESP32-P4C6

# \- \*\*Kamera:\*\* OV5647

# \- \*\*Kameraschnittstelle:\*\* MIPI CSI

# \- \*\*Streaming:\*\* MJPEG über HTTP

# \- \*\*Integrierter Webserver:\*\* Direkt auf dem ESP32-P4C6

# \- \*\*Webbrowser:\*\* Live-Anzeige des Kamerastreams ohne zusätzliche Client-Software

# 

# &#x20;Funktionsweise

# 

# Die OV5647-Kamera ist über die MIPI-CSI-Schnittstelle mit dem ESP32-P4C6 verbunden. Die aufgenommenen Bilder werden als JPEG kodiert und anschließend über eine HTTP-Verbindung an den Client übertragen.

# 

# Der Webserver sendet die einzelnen JPEG-Bilder als Frames innerhalb eines MJPEG-Streams. Ein kompatibler Webbrowser kann diesen Stream empfangen und die Bilder fortlaufend anzeigen.

# 

# Die Bildübertragung erfolgt direkt vom Mikrocontroller zum Browser, ohne dass ein externer Webserver erforderlich ist.

# 

# &#x20;Hardware

# 

# \- ESP32-P4C6

# \- OV5647-Kameramodul mit MIPI-CSI-Anschluss

# \- Netzwerkverbindung für den Zugriff auf den Webserver

# 

# &#x20;Streaming

# 

# Die Übertragung der Bilddaten erfolgt über einen HTTP-Stream. Die JPEG-Daten werden zusammen mit den Frame-Begrenzungen übertragen, sodass der Client die einzelnen Bilder erkennen und darstellen kann.

# 

# &#x20;Projektziel

# 

# Ziel dieses Projekts ist es, eine kompakte Kamera-Streaming-Lösung auf Basis des ESP32-P4C6 zu realisieren. Die Kamera wird direkt über MIPI CSI angebunden, während die Bilddaten über einen integrierten Webserver für den Zugriff per Browser bereitgestellt werden.

# 

# &#x20;ArduinoP4Camera

# 

# An Arduino project for controlling an \*\*OV5647 camera\*\* via the \*\*MIPI CSI interface\*\* using an \*\*ESP32-P4C6\*\* microcontroller.

# 

# The captured camera images are transmitted as an \*\*MJPEG stream over HTTP\*\* and can be displayed directly in a web browser. The web server runs directly on the ESP32-P4C6, eliminating the need for an additional server.

# 

# &#x20;Features

# 

# \- \*\*Microcontroller:\*\* ESP32-P4C6

# \- \*\*Camera:\*\* OV5647

# \- \*\*Camera interface:\*\* MIPI CSI

# \- \*\*Streaming:\*\* MJPEG over HTTP

# \- \*\*Integrated web server:\*\* Hosted directly on the ESP32-P4C6

# \- \*\*Web browser support:\*\* Live camera stream without additional client software

# 

# &#x20;How It Works

# 

# The OV5647 camera is connected to the ESP32-P4C6 via the MIPI CSI interface. The captured images are encoded as JPEG and then transmitted to the client over an HTTP connection.

# 

# The web server sends individual JPEG images as frames within an MJPEG stream. A compatible web browser can receive this stream and continuously display the incoming images.

# 

# The images are streamed directly from the microcontroller to the browser without requiring an external web server.

# 

# &#x20;Hardware Requirements

# 

# \- ESP32-P4C6 microcontroller

# \- OV5647 camera module with a MIPI CSI interface

# \- Network connection to access the web server

# 

# &#x20;Streaming

# 

# Image data is transmitted over an HTTP stream. The JPEG images are sent with frame boundaries, allowing the client to identify and display each individual frame.

# 

# &#x20;Project Goal

# 

# The goal of this project is to implement a compact camera streaming solution based on the ESP32-P4C6. The camera is connected directly via MIPI CSI, while the integrated web server makes the camera stream accessible through a web browser.

# 

