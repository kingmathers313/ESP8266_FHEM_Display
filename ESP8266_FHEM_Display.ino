/**
Copyright (c) 2018 by Philipp Pfeiffer
based upon the Weather Station code using the following license:

The MIT License (MIT)
Copyright (c) 2018 by Daniel Eichhorn
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
See more at https://blog.squix.org
*/


/*****************************
 * Important: see settings.h to configure your settings!!!
 * ***************************/

#include <simpleDSTadjust.h>
//#include <MiniGrafxFonts.h>
//#include <DisplayDriver.h>
//#include <NTPClient.h>
//#include <ESPWiFi.h>
#include "settings.h"

#include <Arduino.h>
//#include <SPI.h>
#include <ESP8266WiFi.h>
#include <XPT2046_Touchscreen.h>
#include "TouchControllerWS.h"
#include "FHEM.h"

/***
 * Install the following libraries through Arduino Library Manager
 * - Mini Grafx by Daniel Eichhorn
 * - simpleDSTadjust by neptune2
 * - FHEM by Philipp Pfeifffer
 ***/


#include <MiniGrafx.h>
#include <ILI9341_SPI.h>

#include "ArialRounded.h"
#include "Icons.h"

#define MINI_BLACK 0
#define MINI_WHITE 1
#define MINI_GREEN 2
#define MINI_BROWN 3

//colors must be hex RGB656, additional color definitions not found in ILI9341_SPI.h: 
#define ILI9341_FHEMGREEN	0x0584		/*   0, 176,  32 */
#define ILI9341_FHEMBEIGE	0xFFFC		/* 248, 252, 224 */
#define ILI9341_FHEMBROWN	0xCE66		/* 200, 204,  48 */

// defines the colors usable in the paletted 16 color frame buffer
uint16_t palette[] = {
	ILI9341_BLACK, // 0
	ILI9341_WHITE, // 1
	ILI9341_FHEMGREEN, // 2
	ILI9341_FHEMBROWN, // 3
};

int SCREEN_WIDTH = 240;
int SCREEN_HEIGHT = 320;
// Limited to 4 colors due to memory constraints
int BITS_PER_PIXEL = 2; // 2^2 =  4 colors

ADC_MODE(ADC_VCC);

ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette);

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
TouchControllerWS touchController(&ts);

void calibrationCallback(int16_t x, int16_t y);
CalibrationCallback calibration = &calibrationCallback;  

simpleDSTadjust dstAdjusted(StartRule, EndRule);

void updateData();
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawWifiQuality();
void drawCurrentStatus();
void drawSecondaryButtons();
String getTime(time_t *timestamp);

FHEM fhemclient(FHEM_SERVER, FHEM_USER, FHEM_PASSWORD);

void Draw(int16_t x, int16_t y, String text, const char *font, uint16_t color, TEXT_ALIGNMENT alignment);
String PowerStatus;
String WaschmaschineStatus;

long lastDownloadUpdate = millis();
long lastTouchscreenPress = -1000;

uint16_t screen = 0;
long timerPress;
bool canBtnPress;
time_t dstOffset = 0;

void connectWifi()
{
	if (WiFi.status() == WL_CONNECTED) return;
	//Manual Wifi
	Serial.print("Connecting to WiFi ");
	Serial.print(WIFI_SSID);
	WiFi.disconnect();
	WiFi.mode(WIFI_STA);
	WiFi.hostname(WIFI_HOSTNAME);
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	int i = 0;
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		if (i > 80) i = 0;
		drawProgress(i, "Connecting to WiFi '" + String(WIFI_SSID) + "'");
		i += 10;
		Serial.print(".");
	}
	drawProgress(100, "Connected to WiFi '" + String(WIFI_SSID) + "'");
	Serial.print("Connected...");
}

void setup()
{
	Serial.begin(115200);

	// The LED pin needs to set HIGH
	// Use this pin to save energy
	// Turn on the background LED
	Serial.println(TFT_LED);
	pinMode(TFT_LED, OUTPUT);
	digitalWrite(TFT_LED, HIGH);    // HIGH to Turn on;

	gfx.init();
	gfx.fillBuffer(MINI_BLACK);
	gfx.commit();

	connectWifi();

	Serial.println("Initializing touch screen...");
	ts.begin();

	Serial.println("Mounting file system...");
	bool isFSMounted = SPIFFS.begin();
	if (!isFSMounted) {
		Serial.println("Formatting file system...");
		drawProgress(50, "Formatting file system");
		SPIFFS.format();
	}
	drawProgress(100, "Formatting done");
	//SPIFFS.remove("/calibration.txt");
	boolean isCalibrationAvailable = touchController.loadCalibration();
	if (!isCalibrationAvailable) {
		Serial.println("Calibration not available");
		touchController.startCalibration(&calibration);
		while (!touchController.isCalibrationFinished()) {
			gfx.fillBuffer(0);
			gfx.setColor(MINI_GREEN);
			gfx.setTextAlignment(TEXT_ALIGN_CENTER);
			gfx.drawString(120, 160, "Please calibrate\ntouch screen by\ntouch point");
			touchController.continueCalibration();
			gfx.commit();
			yield();
		}
		touchController.saveCalibration();
	}

	// update the information
	updateData();
	timerPress = millis();
	canBtnPress = true;
}

long lastDrew = 0;
bool btnClick;
uint8_t MAX_TOUCHPOINTS = 10;
TS_Point points[10];
uint8_t currentTouchPoint = 0;

void loop()
{
	gfx.fillBuffer(MINI_BLACK);
	if (touchController.isTouched(0))
	{
		if (millis() - lastTouchscreenPress > Touchscreen_Delay_MSECS)
		{
			lastTouchscreenPress = millis();
			TS_Point p = touchController.getPoint();

			String x = String(p.x);
			String y = String(p.y);

			String xandy = "x: " + x + ", y: " + y;

			Serial.print(xandy);

			if (screen == 0)
			{
				if (p.y > 105 && p.y < 205)
				{
					screen = 1;
					lastDownloadUpdate = millis(); //so that no update will occur right now, only after interval
				}
			}
			else if (screen == 1)
			{
				bool done = false;

				if (p.y > 200)
				{
					done = true;
					//fhemclient.LoadFromServer("set WaschmaschineDummy startin sofort");
					Serial.write("Start now");
				}
				else if (p.y > 100)
				{
					done = true;
					//fhemclient.LoadFromServer("set WaschmaschineDummy startin 6");
					Serial.write("Start within 6h");
				}
				else
				{
					done = true;
					//fhemclient.LoadFromServer("set WaschmaschineDummy startuntil 14");
					Serial.write("Start until 2pm");
				}

				if (done)
				{
					screen = 0;
					updateData();
				}
			}
		}
	}

	if (screen == 0)
	{
		drawTime();
		drawWifiQuality();
		drawCurrentStatus();
		drawDeviceDetails();
	}
	else if (screen == 1)
	{
		drawSecondaryButtons();
	}

	gfx.commit();

	// Check if we should update  information
	if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS)
	{
		updateData();
	}
}

// Update the internet based information and update screen
void updateData()
{
	lastDownloadUpdate = millis();
	drawProgress(50, "Loading Data...");

	//this gets data from FHEM:
	PowerStatus = fhemclient.LoadFromServer("{ESP8266GetResponse2()}");   //first 3 lines
	WaschmaschineStatus = fhemclient.LoadFromServer("{ESP8266GetResponse()}");  //second 2 lines, with Start button

	gfx.fillBuffer(MINI_BLACK);
	gfx.setFont(ArialRoundedMTBold_14);

	configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);
	while (!time(nullptr)) {
		Serial.print("#");
		delay(100);
	}
	// calculate for time calculation how much the dst class adds.
	dstOffset = UTC_OFFSET * 3600 + dstAdjusted.time(nullptr) - time(nullptr);
	Serial.printf("Time difference for DST: %d", dstOffset);

	screen = 0;
}

void Draw(int16_t x, int16_t y, String text, const char *font, uint16_t color, TEXT_ALIGNMENT alignment)
{
	gfx.setFont(font);
	gfx.setColor(color);
	gfx.setTextAlignment(alignment);
	gfx.drawString(x, y, text);
}

// Progress bar helper
void drawProgress(uint8_t percentage, String text)
{
	gfx.fillBuffer(MINI_BLACK);
	gfx.drawPalettedBitmapFromPgm(24, -30, FHEMLogo);

	Draw(120, 130, "FHEM ", ArialRoundedMTBold_36, MINI_GREEN, TEXT_ALIGN_CENTER);
	Draw(120, 186, text, ArialRoundedMTBold_14, MINI_WHITE, TEXT_ALIGN_CENTER);

	gfx.setColor(MINI_WHITE);
	gfx.drawRect(10, 208, 240 - 20, 15);
	gfx.setColor(MINI_GREEN);
	gfx.fillRect(12, 210, 216 * percentage / 100, 11);

	gfx.commit();
}

// draws the clock
void drawTime()
{
	char time_str[11];
	char *dstAbbrev;
	time_t now = dstAdjusted.time(&dstAbbrev);
	struct tm * timeinfo = localtime(&now);
	
	String date = WDAY_NAMES[timeinfo->tm_wday] + " " + String(timeinfo->tm_mday) + ". " + MONTH_NAMES[timeinfo->tm_mon] + " " + String(1900 + timeinfo->tm_year);
	Draw(120, 6, date, ArialRoundedMTBold_14, MINI_WHITE, TEXT_ALIGN_CENTER);

	sprintf(time_str, "%02d:%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	Draw(120, 20, time_str, ArialRoundedMTBold_36, MINI_WHITE, TEXT_ALIGN_CENTER);
}

// draws current information
void drawCurrentStatus()
{
	Draw(10, 65, PowerStatus, ArialRoundedMTBold_14, MINI_WHITE, TEXT_ALIGN_LEFT);
}

void drawDeviceDetails()
{
	gfx.setColor(MINI_GREEN);
	gfx.drawRect(10, 125, 220, 80);
	Draw(15, 127, WaschmaschineStatus, ArialRoundedMTBold_14, MINI_BROWN, TEXT_ALIGN_LEFT);
	Draw(120, 160, "Start", ArialRoundedMTBold_36, MINI_GREEN, TEXT_ALIGN_CENTER);
}

void drawSecondaryButtons()
{
	Draw(120, 2, "Waschmaschine starten", ArialRoundedMTBold_14, MINI_BROWN, TEXT_ALIGN_CENTER);

	Draw(10, 57, "Sofort", ArialRoundedMTBold_36, MINI_GREEN, TEXT_ALIGN_LEFT);
	Draw(10, 157, "In 6 Stunden", ArialRoundedMTBold_36, MINI_GREEN, TEXT_ALIGN_LEFT);
	Draw(10, 257, "Um 14 Uhr", ArialRoundedMTBold_36, MINI_GREEN, TEXT_ALIGN_LEFT);
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality()
{
	int32_t dbm = WiFi.RSSI();
	if (dbm <= -100)
	{
		return 0;
	}
	else if (dbm >= -50)
	{
		return 100;
	}
	else
	{
		return 2 * (dbm + 100);
	}
}

void drawWifiQuality()
{
	int8_t quality = getWifiQuality();

	Draw(228, 9, String(quality) + "%", ArialMT_Plain_10, MINI_WHITE, TEXT_ALIGN_RIGHT);
	for (int8_t i = 0; i < 4; i++)
	{
		for (int8_t j = 0; j < 2 * (i + 1); j++)
		{
			if (quality > i * 25 || j == 0)
			{
				gfx.setPixel(230 + 2 * i, 18 - j);
			}
		}
	}
}

void calibrationCallback(int16_t x, int16_t y) {
	gfx.setColor(1);
	gfx.fillCircle(x, y, 10);
}

String getTime(time_t *timestamp) {
	struct tm *timeInfo = gmtime(timestamp);

	char buf[6];
	sprintf(buf, "%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min);
	return String(buf);
}
