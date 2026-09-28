#include <SmartThing.h>
#include <DHT.h>

#define DHTTYPE DHT11  

DHT dht(D4, DHTTYPE);

void addSensors();

long filterNan(float value) {
  if (isnan(value)) {
    LOGGER.info("main", "Nan value");
    return 0;
  }

  return value;
}

void setup() {
  dht.begin();

  addSensors();

  if (SmartThing.init("meteo_station")) {
    LOGGER.info("main", "SmartThing successfully initialized");
  } else {
    LOGGER.error("main", "Failed to init SmartThing!");
  }

  if (!dht.read()) {
    LOGGER.error("main", "Failed to read data from sensor!");
  }
}

void loop() {
  SmartThing.loop();
  delay(250);
  dht.read();
}

void addSensors() {
  SensorsManager.add("temperature", []() {
    return filterNan(dht.readTemperature());
  });
  SensorsManager.add("humidity", []() {
    return filterNan(dht.readHumidity());
  });
}
