# Introduction

The purpose of this project is to measure the height level of a salt tank for a water softener. It publishes the distance measured to Home Assistant where it can be converted in percentage and displayed as a lovelace card.

<img width="401" height="206" alt="Screenshot 2025-11-17 at 13 44 14" src="https://github.com/user-attachments/assets/ab8d5b6f-2bfb-4ff2-a8ed-297f53e26a01" />

# Hardware requirements

Any ESP32 - the platformio file is meant for a nodemcu version but it can be adjusted for any board.  

A waterproof ultrasonic sensor with its board: JSN-SR04T  

<img width="400" alt="image" src="https://github.com/user-attachments/assets/5509a7c9-63c7-4969-96d0-f5b378bb670f" />  

Some people have tried with other ultrasonic sensor which ended up being corroded quickly due to the salt environment.

The board needs to be wired to the ESP32 as such:

JSN-SR04T VCC → ESP32 5V  
JSN-SR04T GND → ESP32 GND  
JSN-SR04T Trig → ESP32 GPIO5   
JSN-SR04T Echo → ESP32 GPIO18   

Any support to hold the sensor - note the board + ESP32 should not be put inside the tanks otherwise they will get damaged with the salt.  
In my case, I've used an insert in the plastic of my tank in order to screw a 3d printed holder for the sensor, in a way which set it in the middle. 

<img width="401" src="https://github.com/user-attachments/assets/344e1578-9f06-4906-b04f-ab54837d772d" />

# Software

Rename secrets_template.h into secrets.h and enter your credentials (wifi + MQTT settings)

Compile it: 

``` shell
pio run -t clean
pio run -e esp32
pio run -t upload -e esp32
pio device monitor -b 115200
```

Use the Yaml files in docs folder for the Home Assistant setup: salt_level.yaml needs to be in a folder loaded by the configuration
The yaml in configuration.yaml needs to be copied in the section template of your existing setup.  You will need to adjust the max depth to your tanks dimensions.

The lovelace card can be setup using this:  
``` yaml
type: gauge
entity: sensor.salt_level
name: Salt Level
min: 0
max: 100
unit: "%"
severity:
  green: 60
  yellow: 35
  red: 0
```


