#include <SmartThing.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>
#include <Adafruit_BMP085.h>

#include "icon.h"

#define DHTTYPE DHT22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin
#define SCREEN_ADDRESS 0x3C

DHT dht(D5, DHTTYPE);
Adafruit_BMP085 bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int dhtTemp = 0;
int bmpTemp = 0;
int hum = 0;
int pressure = 0;

void setupSmt();
void drawDisplay();

float filterNan(float value, int defaultValue) {
  if (isnan(value)) {
    return defaultValue;
  }

  return value;
}

void setup() {
  dht.begin();
  if (!dht.read()) {
    LOGGER.error("main", "Failed to read data from sensor!");
  }
  if (!bmp.begin()) {
    LOGGER.error("main", "Failed to init bmp sensor");
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    LOGGER.error("main", "SSD1306 allocation failed");
  }

  display.setTextColor(WHITE);
  display.clearDisplay();

  setupSmt();

  if (SmartThing.init("meteo_station")) {
    LOGGER.info("main", "SmartThing successfully initialized");
  } else {
    LOGGER.error("main", "Failed to init SmartThing!");
  }

  ArduinoOTA.onStart([]() {
    display.clearDisplay();
    display.setTextSize(1);
    display.drawBitmap(0, 0, firmware_update_icon_16_32, 16, 32, WHITE);
    display.setCursor(20, 7);
    display.print("Firmware update");
    display.setCursor(20, 17);
    display.print("Please wait :)");
    display.display();
  });
  ArduinoOTA.onEnd([]() {
    display.clearDisplay();
    display.setTextSize(1);
    display.drawBitmap(0, 0, firmware_update_icon_16_32, 16, 32, WHITE);
    display.setCursor(20, 7);
    display.print("Update finished");
    display.setCursor(20, 17);
    display.print("Rebooting now");
    display.display();
  });
}

void loop() {
  SmartThing.loop();
  
  bmpTemp = bmp.readTemperature();
  dhtTemp = filterNan(dht.readTemperature(), bmpTemp);
  pressure = bmp.readPressure() * 0.00750063755419211; // мм рт. ст.
  hum = filterNan(dht.readHumidity(), 0);

  drawDisplay();

  delay(250);
}

void drawDisplay() {
  display.clearDisplay();

  display.setTextSize(2);
  display.drawBitmap(0, 0, temp_icon_16_16, 16, 16, WHITE);
  display.setCursor(18, 0);
  display.print(dhtTemp);
  display.print("C");

  display.drawBitmap(60, 0, humid_icon_16_16, 16, 16, WHITE);
  display.setCursor(78, 0);
  display.print(hum);
  display.print("%");

  display.drawBitmap(0, 24, temp_icon_16_16, 16, 16, WHITE);
  display.setCursor(18, 24);
  display.print(bmpTemp);
  display.print("C");

  display.drawBitmap(60, 24, pressure_icon_16x16, 16, 16, WHITE);
  display.setCursor(78, 24);
  display.print(pressure);

  display.setTextSize(1);
  display.drawBitmap(8, 48, wifi_icon_8_8, 8, 8, WHITE);
  display.setCursor(18, 48);
  if (SmartThing.wifiConnected()) {
    display.print(SmartThing.getIp());
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    display.print("WiFi not connected");
  }

  display.display();
}

void setupSmt() {
  SensorsManager.add("dht_temperature", []() {
    return dhtTemp;
  });
  SensorsManager.add("humidity", []() {
    return hum;
  });
  SensorsManager.add("bmp_temperature", []() {
    return bmpTemp;
  });
  SensorsManager.add("pressure", []() {
    return pressure;
  });
  ActionsManager.add("led", "Turn led on/off", []() {
      digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) == HIGH ? LOW : HIGH);
      return true;
  });
}
