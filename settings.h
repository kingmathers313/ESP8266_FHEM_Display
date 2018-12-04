/**
Copyright (c) 2018 by Philipp Pfeiffer
based upon the Weather Station code using the following license:

The MIT License (MIT)
Copyright (c) 2015 by Daniel Eichhorn
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
See more at http://blog.squix.ch
*/

#include <simpleDSTadjust.h>

// Setup
const char* WIFI_SSID = "ssid";
const char* WIFI_PASS = "password";
const char* WIFI_HOSTNAME = "ESP8266_FHEM_Display";

const String FHEM_SERVER = "http://192.168.1.1:8083/fhem"; //full url, must include port
const String FHEM_USER = "Username"; //leave blank (const String FHEM_User = "";) if you use no Basic Auth
const String FHEM_PASSWORD = "Password";

const int Touchscreen_Delay_MSECS = 1000; //ignore inputs for this long after first input (ignores accidential second touch event)
const int UPDATE_INTERVAL_SECS = 30; // Update every x sec

// Adjust according to your language
const String WDAY_NAMES[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
const String MONTH_NAMES[] = {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};

#define UTC_OFFSET +1
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600}; // Central European Summer Time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Central European Time = UTC/GMT +1 hour

// Settings for Boston
// #define UTC_OFFSET -5
// struct dstRule StartRule = {"EDT", Second, Sun, Mar, 2, 3600}; // Eastern Daylight time = UTC/GMT -4 hours
// struct dstRule EndRule = {"EST", First, Sun, Nov, 1, 0};       // Eastern Standard time = UTC/GMT -5 hour

// change for different ntp (time servers)
#define NTP_SERVERS "0.ch.pool.ntp.org", "1.ch.pool.ntp.org", "2.ch.pool.ntp.org"
// #define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"

// Pins for the ILI9341
#define TFT_DC D2
#define TFT_CS D1
#define TFT_LED D8

#define HAVE_TOUCHPAD
#define TOUCH_CS D3
#define TOUCH_IRQ  D4


/***************************
 * End Settings
 **************************/
