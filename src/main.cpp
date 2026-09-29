#include <SmartThing.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>

#define DHTTYPE DHT11  

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 23
#define OLED_RESET -1 // Reset pin
#define SCREEN_ADDRESS 0x3C

const unsigned char wifi_icon [] PROGMEM = {
  0x3C, 0x42, 0x18, 0x24, 0x00, 0x18, 0x18, 0x00
};
const unsigned char temp_icon [] PROGMEM = {
  0x18, 0x18, 0x18, 0x18, 0x3C, 0x7E, 0x7E, 0x3C
};
const unsigned char humid_icon [] PROGMEM = {
  0x10, 0x38, 0x7C, 0xFE, 0xFE, 0xEE, 0x7C, 0x38
};

DHT dht(D5, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int temp = 0;
int hum = 0;

void addSensors();
void drawDisplay();

long filterNan(float value) {
  if (isnan(value)) {
    return 0;
  }

  return value;
}

void setup() {
  dht.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    LOGGER.error("main", "SSD1306 allocation failed");
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);

  addSensors();

  if (SmartThing.init("meteo_station")) {
    LOGGER.info("main", "SmartThing successfully initialized");
  } else {
    LOGGER.error("main", "Failed to init SmartThing!");
  }

  ArduinoOTA.onStart([]() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Firmware update");
    display.setCursor(0, 16);
    display.print("Please wait :)");
    display.display();
  });

  if (!dht.read()) {
    LOGGER.error("main", "Failed to read data from sensor!");
  }
}

void loop() {
  SmartThing.loop();
  
  temp = filterNan(dht.readTemperature());
  hum = filterNan(dht.readHumidity());

  drawDisplay();

  delay(250);
}

void drawDisplay() {
  display.clearDisplay();

  display.drawBitmap(0, 0, temp_icon, 8, 8, WHITE);
  display.setCursor(10, 0);
  display.print(temp);
  display.print("C");

  display.drawBitmap(32, 0, humid_icon, 8, 8, WHITE);
  display.setCursor(42, 0);
  display.print(hum);
  display.print("%");

  display.drawLine(0, 11, SCREEN_WIDTH, 11, WHITE);

  display.drawBitmap(0, 16, wifi_icon, 8, 8, WHITE);
  display.setCursor(10, 16);
  if (SmartThing.wifiConnected()) {
    display.print(SmartThing.getIp());
  } else {
    display.print("WiFi not connected");
  }

  display.display();
}

void addSensors() {
  SensorsManager.add("temperature", []() {
    return temp;
  });
  SensorsManager.add("humidity", []() {
    return hum;
  });
}
