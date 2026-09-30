#include <SmartThing.h>
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoOTA.h>

#define DHTTYPE DHT11  

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1 // Reset pin
#define SCREEN_ADDRESS 0x3C

const unsigned char wifi_icon [] PROGMEM = {
  0x3C, 0x42, 0x18, 0x24, 0x00, 0x18, 0x18, 0x00
};
const unsigned char temp_icon [] PROGMEM = {
  0x04, 0x00, //   *  
  0x04, 0x00, //   *  
  0x0A, 0x00, //  * * 
  0x0A, 0x00, //  * * 
  0x0A, 0x00, //  * * 
  0x0E, 0x00, //  *** 
  0x0E, 0x00, //  *** 
  0x0E, 0x00, //  *** 
  0x0E, 0x00, //  *** 
  0x1F, 0x80, // *****
  0x1B, 0x80, // ** **
  0x1B, 0x80, // ** **
  0x1F, 0x80, // *****
  0x0E, 0x00, //  *** 
  0x04, 0x00, //   *  
  0x00, 0x00  //      
};
const unsigned char humid_icon [] PROGMEM = {
  0x00, 0x00, //                 
  0x01, 0x80, //        ##       
  0x03, 0xC0, //       ####      
  0x03, 0xC0, //       ####      
  0x07, 0xE0, //      ######     
  0x07, 0xE0, //      ######     
  0x0F, 0xF0, //     ########    
  0x0F, 0xF0, //     ########    
  0x1F, 0xF8, //    ##########   
  0x1F, 0xF8, //    ##########   
  0x3F, 0xFC, //   ############  
  0x3F, 0xFC, //   ############  
  0x1F, 0xF8, //    ##########   
  0x0F, 0xF0, //     ########    
  0x07, 0xE0, //      ######     
  0x00, 0x00  //
};

const unsigned char firmware_update[] PROGMEM = {
  0x00, 0x00, //                 
  0x03, 0xc0, //       ****      
  0x03, 0xc0, //       ****      
  0x03, 0xc0, //       ****      
  0x03, 0xc0, //       ****      
  0x03, 0xc0, //       ****      
  0x03, 0xc0, //       ****      
  0x0f, 0xf0, //     ********    
  0x07, 0xe0, //      ******     
  0x03, 0xc0, //       ****      
  0x01, 0x80, //        **       
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x1f, 0xf8, //    **********   
  0x10, 0x08, //    *        *   
  0x10, 0x08, //    *        *   
  0x10, 0x08, //    *        *   
  0x17, 0xe8, //    * ****** *   
  0x17, 0xe8, //    * ****** *   
  0x17, 0xe8, //    * ****** *   
  0x17, 0xe8, //    * ****** *   
  0x17, 0xe8, //    * ****** *   
  0x17, 0xe8, //    * ****** *   
  0x17, 0xe8, //    * ****** *   
  0x17, 0xe8, //    * ****** *   
  0x10, 0x08, //    *        *   
  0x10, 0x08, //    *        *   
  0x1f, 0xf8, //    **********   
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00, //                 
  0x00, 0x00  //  
};

DHT dht(D5, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int temp = 0;
int hum = 0;

void setupSmt();
void drawDisplay();

long filterNan(float value) {
  if (isnan(value)) {
    return 0;
  }

  return value;
}

void setup() {
  dht.begin();
  if (!dht.read()) {
    LOGGER.error("main", "Failed to read data from sensor!");
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
    display.drawBitmap(0, 0, firmware_update, 16, 32, WHITE);
    display.setCursor(20, 7);
    display.print("Firmware update");
    display.setCursor(20, 17);
    display.print("Please wait :)");
    display.display();
  });
  ArduinoOTA.onEnd([]() {
    display.clearDisplay();
    display.setTextSize(1);
    display.drawBitmap(0, 0, firmware_update, 16, 32, WHITE);
    display.setCursor(20, 7);
    display.print("Update finished");
    display.setCursor(20, 17);
    display.print("Rebooting now");
    display.display();
  });
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

  display.setTextSize(2);
  display.drawBitmap(0, 0, temp_icon, 16, 16, WHITE);
  display.setCursor(18, 0);
  display.print(temp);
  display.print("C");

  display.drawBitmap(70, 0, humid_icon, 16, 16, WHITE);
  display.setCursor(90, 0);
  display.print(hum);
  display.print("%");

  display.setTextSize(1);
  display.drawBitmap(8, 24, wifi_icon, 8, 8, WHITE);
  display.setCursor(18, 24);
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
    return temp;
  });
  SensorsManager.add("humidity", []() {
    return hum;
  });
  ActionsManager.add("led", "Turn led on/off", []() {
      digitalWrite(LED_BUILTIN, digitalRead(LED_BUILTIN) == HIGH ? LOW : HIGH);
      return true;
  });
}
