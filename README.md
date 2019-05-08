# Heart Rate Arduino
Arduino-based group project to measure and run statistics on your heart rate from a handheld sensor

## Project Summary
A systems programming course required students to combine arduino programming with file data architecture. Initially, using a heart rate sensor, data was printed from another Arduino (connected through I<sup>2</sup>C) onto an LCD display. Afterwards, user input was used to pause, resume, exit, show histogram data, and eventually statistics. 

## Hardware Key Components
- 2 Arduinos connected through a breadboard
- Blinking LED to match heart rate
- Heart Rate sensor
- LCD display with matching wiring
- Potentiometer to manage power through the LCD
- Multiple Resistors

## Group Member Responsibilities
[Sam Bunger](https://github.com/sam-bunger)  
- Multithreaded user interface created with forking 
- Data recovery and saving
- Connecting master arduino to request gathered data

[Samuil Agranovich](https://github.com/soviet-potato)  
- Breadboard and Arduino diagrams
- Explanations and Write-Ups
- Videos for demo

[Kevin Deems](https://github.com/kevindweb)  
- Hardware wiring and displays
- Data collection and parsing
- Statistics and regression analysis
