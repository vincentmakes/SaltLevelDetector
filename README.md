# Introduction

The purpose of this project is to measure the height level of a salt tank for a water softener. It publishes the distance measured to Home Assistant where it can be converted in percentage and displayed as a lovelace card.

For people not interested in using Home Assistant but want a standalone setup, one can setup a Bark key to receive a notification when the salt level goes below a threshold.
Bark is a free iOS app which can be installed on any Apple mobile device and is meant to receive notifications. An alternative for Android users will be available soon.

A WebUI also displays the level directly.

<img width="401" height="206" alt="Screenshot 2025-11-17 at 13 44 14" src="https://github.com/user-attachments/assets/ab8d5b6f-2bfb-4ff2-a8ed-297f53e26a01" />

# Hardware requirements

Any ESP32 - the platformio file is meant for a nodemcu version but it can be adjusted for any board.  

A waterproof ultrasonic sensor with its board: JSN-SR04T  

<img width="400" alt="image" src="https://github.com/user-attachments/assets/5509a7c9-63c7-4969-96d0-f5b378bb670f" />  

Some people have tried with other ultrasonic sensor which ended up being corroded quickly due to the salty environment.

The board needs to be wired to the ESP32 as such:

JSN-SR04T VCC → ESP32 5V  
JSN-SR04T GND → ESP32 GND  
JSN-SR04T Trig → ESP32 GPIO5   
JSN-SR04T Echo → ESP32 GPIO18   

Any support to hold the sensor - note the board + ESP32 should not be put inside the tanks otherwise they will get damaged with the salt.  
In my case, I've used an insert in the plastic of my tank in order to screw a 3d printed holder for the sensor, in a way which set it in the middle. A piece of wood with a hole, secured on the tank would also work fine.

<img width="401" src="https://github.com/user-attachments/assets/344e1578-9f06-4906-b04f-ab54837d772d" />



# Software
## Quick Start
Rename secrets_template.h into secrets.h and enter your credentials : Wifi + MQTT settings + Bark settings.

MQTT is used if you want to integrate with Home Assistant and display the salt level in a lovelace card.  

If you don't have Home Assistant and prefer to keep things simple but still want to receive a notification when the salt level is low, simply install Bark on your mobile and get the API key from there. Define your minimum level in centimeters from the top of the sensor (45cm by default). Bark will send the notification only once and will only reset once the tank is 3cm above the threshold again.

You can change the frequency of the measurement: by default it is set in constant.h at 1hr.  

Compile the code and upload it to the esp32: 

``` shell
pio run -t clean
pio run -e esp32
pio run -t upload -e esp32
pio device monitor -b 115200
```

## Home Assistant Setup (Optional, requires MQTT)
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
## Settings in the WebUI

Browsing to the IP address of the ESP32 or to http://saltlevel-esp32.local/ you will find a webpage to display the status of the salt level, as well as adjust the settings (retained at reboot). The language of the webpage can be switched to french.

The intent behind this was to make it accessible for people without a Home Assistant setup and needed some autonomy in adjustment of the settings without having to recompile the firmware.



<img width="462"  src="https://github.com/user-attachments/assets/bed83605-f0dd-4e5d-b5df-9785d36035b2" /> <br/>
<img width="462"   src="https://github.com/user-attachments/assets/6025a7d5-7c94-4358-bde8-003e0832056f" />





