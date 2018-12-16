# Garage Door Checker

Application reports garage door open/closed. It is based on [Wemos D1 mini lite](https://wiki.wemos.cc/products:d1:d1_mini_lite) and [Ultrasonic Sensor HC-SR04](https://randomnerdtutorials.com/complete-guide-for-ultrasonic-sensor-hc-sr04/) and optional [Nokia 5110 LCD display](https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library).

It is reported via [IFTTT](https://ifttt.com) to whatever endpoint.

## Development environment

After basic start with ~~[Arduino IDE](https://www.arduino.cc/en/main/software)~~ I've switched to absolutely fantastic combination of [VSCode](https://code.visualstudio.com/) with [PlatformIO](https://platformio.org/). __It's so cool!__ It is a really pleasure to 

There are several libraries used in project, most of them imported via PlatformIO, some are included in the project, as some changes had to be done.

- [NewPing for ESP8266](https://github.com/jshaw/NewPingESP8266), I believe it's a modification of [Teckel's implementation](https://bitbucket.org/teckel12/arduino-new-ping/downloads).
- [PCD8544 - Nokia 5110 LCD](https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library) - there is a [merge request](https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library/pull/27) - the code included in the project is with the changes taken from merge request.

## Hardware 
![Breadboard](doc/breadboard.jpg)

__Connections__
```
 WeMos D1 Mini   Nokia 5110    Description
 (ESP8266)       PCD8544 LCD

 D2 (GPIO4)      0 RST         Output from ESP to reset display
 D1 (GPIO5)      1 CE          Output from ESP to chip select/enable display
 D6 (GPIO12)     2 DC          Output from display data/command to ESP
 D7 (GPIO13)     3 Din         Output from ESP SPI MOSI to display data input
 D5 (GPIO14)     4 Clk         Output from ESP SPI clock
 3V3 / 5V        5 Vcc         3.3V / 5V from ESP to display
 D0 (GPIO16)     6 BL          3.3V to turn backlight on, or PWM
 G               7 Gnd         Ground
```

## Configuration

The file _data/configuration.json_ contains secrets - like ssid, password and ifttt api key.

## SSL connection

The fingerprint has to be provided to establish connection with https server

```bash
openssl s_client -showcerts -connect maker.ifttt.com:443 </dev/null 2>/dev/null|openssl x509 -outform PEM | openssl x509 -noout -fingerprint -sha1
```
