#include <FS.h>

class PirSensor {
  private:
    byte pin;
    String name;
    bool currentState;
    bool prevState;

  public:
    PirSensor() {};
    void init(byte pin, String name = "PIRsensor");
    void setName(String name);
    String getName();
    bool getState();
    bool syncState();
    void save();
    void load();
};

void PirSensor::init(byte pin, String name) {
  this->pin = pin;
  this->name = name;
  pinMode(pin, INPUT);
}

void PirSensor::setName(String name) {
  this->name = name;
}

String PirSensor::getName() {
  return name;
}

bool PirSensor::getState() {
  return currentState;
}

bool PirSensor::syncState() {
  prevState = currentState;
  currentState = digitalRead(pin) == HIGH;
  return currentState != prevState;
}

void PirSensor::save() {
  String fileName = "/sensor_" + String(pin) + ".txt";
  File file = SPIFFS.open(fileName, "w");
  if (!file) {
    return;
  }
  file.print(name);
  file.close();
}

void PirSensor::load() {
  String fileName = "/sensor_" + String(pin) + ".txt";
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    return;
  }
  name = file.readStringUntil('\n');
  file.close();
}
