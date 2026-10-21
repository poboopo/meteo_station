#include <SmartThing.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>

#include "icon.h"

#define DHTTYPE DHT22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin
#define SCREEN_ADDRESS 0x3C

#define UPDATE_DELAY 10000

#define SHOW_UPDATE_CONF "showUpdate"

DHT dht(D5, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int temperature = 0;
int hum = 0;
long lastUpdate = 0;
bool showUpdate = true;

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
  temperature = filterNan(dht.readTemperature(), 0);
  hum = filterNan(dht.readHumidity(), 0);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    LOGGER.error("main", "SSD1306 allocation failed");
  }

  display.setTextColor(WHITE);
  display.clearDisplay();

  setupSmt();

  if (SmartThing.init("meteo_station")) {
    LOGGER.info("main", "SmartThing successfully initialized");
    showUpdate = ConfigManager.getBool(SHOW_UPDATE_CONF, true);
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
  
  if (millis() - lastUpdate > UPDATE_DELAY) {
    temperature = filterNan(dht.readTemperature(), 0);
    hum = filterNan(dht.readHumidity(), 0);
    lastUpdate = millis();
  }

  drawDisplay();

  delay(150);
}

void drawDisplay() {
  display.clearDisplay();

  if (showUpdate) {
    int width = (millis() - lastUpdate) * 128 / UPDATE_DELAY;
    display.fillRect(0, 24, width, 2, WHITE);
  }

  display.drawBitmap(0, 0, temp_icon_18_24, 18, 24, WHITE);
  display.setCursor(18, 1);
  display.setTextSize(3);
  display.print(temperature);
  display.setTextSize(1);
  display.setCursor(display.getCursorX(), 16);
  display.print("C");

  display.drawBitmap(64, 0, humid_icon_18_24, 18, 24, WHITE);
  display.setCursor(80, 1);
  display.setTextSize(3);
  display.print(hum);
  display.setTextSize(1);
  display.setCursor(display.getCursorX(), 16);
  display.print("%");

  display.drawBitmap(0, 48, wifi_icon_16_16, 16, 16, WHITE);
  display.setTextSize(1);
  display.setCursor(18, 54);
  if (SmartThing.wifiConnected()) {
    display.print(SmartThing.getIp());
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    display.print("WiFi not connected");
  }

  display.display();
}

void setupSmt() {
  SensorsManager.add("temperature", []() {
    return temperature;
  });
  SensorsManager.add("humidity", []() {
    return hum;
  });
  ActionsManager.add("led", "Turn led on/off", []() {
      digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) == HIGH ? LOW : HIGH);
      return true;
  });
  ConfigManager.add(SHOW_UPDATE_CONF);
  ConfigManager.onConfigUpdate([]() {
    showUpdate = ConfigManager.getBool(SHOW_UPDATE_CONF, true);
  });
}
