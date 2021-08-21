/**
   @file RAK12500_GPS_ZOE-M8Q_IIC.ino
   @author rakwireless.com
   @brief use I2C get and parse gps data
   @version 0.1
   @date 2021-1-28
   @copyright Copyright (c) 2021
**/

#include <Wire.h> // Needed for I2C to GNSS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
// http://librarymanager/All#SparkFun_u-blox_GNSS
#include "SSD1306Ascii.h"
// http://librarymanager/All#SSD1306Ascii
#include "SSD1306AsciiWire.h"
#include <DS3231M.h> // Include the DS3231M RTC library

DS3231M_Class DS3231M; ///< Create an instance of the DS3231M class
bool TimeAdjusted = false;

#define I2C_ADDRESS 0x3C
#define RST_PIN -1
#define OLED_FORMAT &Adafruit128x64
SSD1306AsciiWire oled;

SFE_UBLOX_GNSS g_myGNSS;
long g_lastTime = 0;
// Simple local timer. Limits amount of I2C traffic to u-blox module.

void setup() {
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, 0);
  delay(1000);
  digitalWrite(WB_IO2, 1);
  delay(1000);
  Wire.begin();
  oled.begin(OLED_FORMAT, I2C_ADDRESS);
  oled.setFont(System5x7);
  oled.setScrollMode(SCROLL_MODE_AUTO);
  oled.println("\n\n\nWe're live!");
  // Initialize Serial for debug output
  time_t timeout = millis();
  Serial.begin(115200);
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  Serial.println("GPS ZOE-M8Q Example(I2C)");
  oled.println("GPS ZOE-M8Q (I2C)");
  if (g_myGNSS.begin() == false) {
    //Connect to the u-blox module using Wire port
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    oled.println(F("u-blox GNSS not detected.\nCheck wiring.\nFreezing."));
    while (1);
  }
  g_myGNSS.setI2COutput(COM_TYPE_UBX);
  //Set the I2C port to output UBX only (turn off NMEA noise)
  g_myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
  //Save (only) the communications port settings to flash and BBR
  while (!DS3231M.begin()) {
    // Initialize RTC communications
    Serial.println(F("Unable to find DS3231MM. Checking again in 3s."));
    delay(3000);
  } // of loop until device is located
  DS3231M.pinSquareWave(); // Make INT/SQW pin toggle at 1Hz
  Serial.println(F("DS3231M initialized."));
  DS3231M.adjust(); // Set to library compile Date/Time
  Serial.print(F("DS3231M chip temperature is "));
  Serial.print(DS3231M.temperature() / 100.0, 1); // Value is in 100ths of a degree
  Serial.println("\xC2\xB0C");
}

void loop() {
  if (millis() - g_lastTime > 10000) {
    long latitude = g_myGNSS.getLatitude();
    long longitude = g_myGNSS.getLongitude();
    long altitude = g_myGNSS.getAltitude();
    long speed = g_myGNSS.getGroundSpeed();
    long heading = g_myGNSS.getHeading();
    byte SIV = g_myGNSS.getSIV();
    char buff[24];
    sprintf(buff, "SIV: %d", SIV);
    oled.println(buff);
    Serial.println(buff);
    if (SIV > 0) {
    // No point if there are no satellites in view
      sprintf(buff, "Lat: %3.7f", (latitude / 1e7));
      oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Long: %3.7f", (longitude / 1e7));
      oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Alt: %3.3f m", (altitude / 1e3));
      oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Speed: %3.3f m/s", (speed / 1e3));
      oled.println(buff);
      Serial.println(buff);
      sprintf(buff, "Heading: %3.7f", (heading / 1e5));
      oled.println(buff);
      Serial.println(buff);
    }
    if (g_myGNSS.getTimeValid()) {
      uint8_t hour = (g_myGNSS.getHour() + 8) % 24;
      // Adjust for HK Time
      // Warning: this only adjusts the time. Overflow will rotate back to 0
      // [this is 24-hour system remember] but it doesn't take care of the date.
      // which is fine here, but you might need to use a library to adjust properly.
      sprintf(buff, "GPS Time: %02d:%02d:%02d HKT", hour, g_myGNSS.getMinute(), g_myGNSS.getSecond());
      oled.println(buff);
      if (!TimeAdjusted) {
        // if we haven't adjusted the time, let's do so.
        DS3231M.adjust(DateTime(g_myGNSS.getYear(), g_myGNSS.getMonth(), g_myGNSS.getDay(),
          g_myGNSS.getHour(), g_myGNSS.getMinute(), g_myGNSS.getSecond()));
        TimeAdjusted = true;
      } else {
        // Do we need to adjust the time?
        // Check for excessive difference between GNSS time and RTC time
        if (g_myGNSS.getYear() != now.year() || g_myGNSS.getMonth() != now.month() || g_myGNSS.getDay() != now.day()
            || g_myGNSS.getHour() != now.hour || g_myGNSS.getMinute() != now.minute) {
          Serial.print("Readjusting the RTC");
          DS3231M.adjust(DateTime(g_myGNSS.getYear(), g_myGNSS.getMonth(), g_myGNSS.getDay(), g_myGNSS.getHour(), g_myGNSS.getMinute(), g_myGNSS.getSecond()));
        }
      }
    }
    DateTime now = DS3231M.now();
    char output_buffer[32];
    sprintf(output_buffer, "RTC DateTime: %04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    Serial.println(output_buffer);
  }
  g_lastTime = millis(); //Update the timer
}
