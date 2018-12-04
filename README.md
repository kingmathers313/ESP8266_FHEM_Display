# ESP8266 FHEM Display

ESP8266 FHEM Display using the ThingPulse ESP8266 Color Display Kit with ILI9341 240x320 TFT display

## Hardware

This project is built upon the [ThingPulse ESP8266 Weather Station Color](https://github.com/ThingPulse/esp8266-weather-station-color) which uses the [ThingPulse ESP8266 Color Display Kit](https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/). Please use these resources for assembling the display or for further information.

## What is this?

This is a modification of the Weather Station to interact with a FHEM Server instead. It can show information from a FHEM Server and it can react to touch events and send commands to the FHEM Server. It requires a FHEM sever that is not protected by SSL or a certificate to work.

It uses the [FHEM Arduino Library](https://github.com/kingmathers313/FHEM_Arduino)

## License

The code in this repository is licensed under [MIT](https://en.wikipedia.org/wiki/MIT_License), a short and simple permissive license with conditions only requiring preservation of copyright and license notices. Thus, you're free to fork the project and use the code for your own projects as long as you keep the copyright notices in place.
