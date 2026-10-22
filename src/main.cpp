#include <SmartThing.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>

#include "icon.h"

#if defined OUTDOOR_METEO_STATION && OUTDOOR_METEO_STATION == 1
  #define ROOM_METEO_STATION 0
  #define DHTTYPE DHT22
  #define DEVICE_TYPE "meteo_station"
#elif defined ROOM_METEO_STATION && ROOM_METEO_STATION == 1
  #define OUTDOOR_METEO_STATION 0
  #define DHTTYPE DHT11
  #define DEVICE_TYPE "room_meteo_station"
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin
#define SCREEN_ADDRESS 0x3C

#define TOUCH_PIN D6
#define AWAKE_TIME_MS 10000
#define UPDATE_DELAY 10000

#define SHOW_UPDATE_CONF "showUpdate"

DHT dht(D5, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int temperature = 0;
int hum = 0;
#if ROOM_METEO_STATION == 1
  int ppm = 0;
#endif

bool pressed = false;
bool displayEnabled = true;
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

void readSensors() {
  temperature = filterNan(dht.readTemperature(), 0);
  hum = filterNan(dht.readHumidity(), 0);

  #if ROOM_METEO_STATION == 1
    ppm = (1023 - analogRead(A0)) * 100 / 1023;
  #endif
}

void setup() {
  pinMode(TOUCH_PIN, INPUT);

  dht.begin();
  if (!dht.read()) {
    LOGGER.error("main", "Failed to read data from sensor!");
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    LOGGER.error("main", "SSD1306 allocation failed");
  }

  delay(5000);
  readSensors();

  display.setTextColor(WHITE);
  display.clearDisplay();

  setupSmt();

  if (SmartThing.init(DEVICE_TYPE)) {
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
    display.drawRect(0, 32, 128, 8, WHITE);
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
  ArduinoOTA.onProgress([](unsigned int total, unsigned int size) {
    unsigned int width = (total) * 128 / size;
    display.fillRect(0, 32, width, 8, WHITE);
    display.display();
  });
}

void loop() {
  SmartThing.loop();
  
  if (digitalRead(TOUCH_PIN)) {
    if (!pressed) {
      displayEnabled = !displayEnabled;
      pressed = true;
    }
  } else if (pressed) {
    pressed = false;
  }

  if (millis() - lastUpdate > UPDATE_DELAY) {
    readSensors();
    lastUpdate = millis();
  }

  if (displayEnabled) {
    drawDisplay();
  } else {
    display.clearDisplay();
    display.display();
  }

  delay(150);
}

void drawSensor(uint16_t x, uint16_t y, const uint8_t *icon, int &value, char units) {
  display.drawBitmap(x, y, icon, 18, 24, WHITE);
  display.setCursor(x + 18, y + 1);
  display.setTextSize(3);
  display.print(value);
  display.setTextSize(1);
  display.setCursor(display.getCursorX(), y + 16);
  display.print(units);
}

void drawDisplay() {
  display.clearDisplay();

  if (showUpdate) {
    int width = (millis() - lastUpdate) * 128 / UPDATE_DELAY;
    display.fillRect(0, 0, width, 2, WHITE);
  }

  drawSensor(0, 3, temp_icon_18_24, temperature, 'C');
  drawSensor(64, 3, humid_icon_18_24, hum, '%');

  #if ROOM_METEO_STATION == 1
    drawSensor(0, 28, air_icon_18_24, ppm, '%');
  #endif

  display.drawBitmap(0, 54, wifi_icon_8_8, 8, 8, WHITE);
  display.setTextSize(1);
  display.setCursor(8, 54);
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
  #if ROOM_METEO_STATION == 1
    SensorsManager.add("quality", []() {
      return ppm;
    });
  #endif

  ActionsManager.add("led", "Turn led on/off", []() {
      digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) == HIGH ? LOW : HIGH);
      return true;
  });

  ConfigManager.add(SHOW_UPDATE_CONF);
  ConfigManager.onConfigUpdate([]() {
    showUpdate = ConfigManager.getBool(SHOW_UPDATE_CONF, true);
  });
}
