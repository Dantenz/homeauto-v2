# homeauto-v2
Simple project to read various sensors using an Arduino Portenta Machine Control and publish changes to mqtt topics.

## Getting Going
- Install Platformio on VSCode
- Set up a new Platformio project - ensure a platformio.ini file is generated
- Define `build_flags` within ini for things like IP, board id, etc. Flags can be found at top of main.cpp

## Customising
The code is designed to work with a Portenta Machine Control - I've found this is the quickest way to just get things 
going for sensors that run on 24v power and generate 0-10v analogue + digital outputs. Most boards should work with very little
changes:
- Remove reference to Machine Control header file
- Change pin definitions as desired