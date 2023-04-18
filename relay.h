#include <FS.h>

class Relay 
{
  public:
    Relay();
    void init(byte pin = 0, bool initialState = LOW, String name = "Relay");
    void setState(bool state);
    void setName(String name);
    bool getState();
    String getName();
    byte getPin();
    bool save();
    bool load();
  private:
    String name;
    bool state;
    byte pin;
};

// конструктор класса
Relay::Relay()
{
  
}

void Relay::init(byte pin, bool initialState, String name)
{
  this->pin = pin;
  pinMode(pin, OUTPUT);
  this->name = name;
  this->state = initialState;
  digitalWrite(pin, state);
}

// метод для установки состояния реле
void Relay::setState(bool state)
{
  digitalWrite(pin, state ? LOW : HIGH);
  this->state = state;
}

// метод для установки имени реле
void Relay::setName(String name)
{
  this->name = name;
}

// метод для получения текущего состояния реле
bool Relay::getState()
{
  return state;
}

// метод для получения имени реле
String Relay::getName()
{
  return name;
}

// метод для получения номера пина, к которому подключено реле
byte Relay::getPin()
{
  return pin;
}

// метод для сохранения текущих значений в файл
bool Relay::save()
{
  String filename = "/relay" + String(pin) + ".txt";
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    return false;
  }
  file.println(name);
  file.println(state);
  file.println(pin);
  file.close();
  return true;
}

// метод для загрузки значений из файла
bool Relay::load()
{
  String filename = "/relay" + String(pin) + ".txt";
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    return false;
  }
  String name = file.readStringUntil('\n');
  bool state = file.readStringUntil('\n').toInt();
  byte pin = file.readStringUntil('\n').toInt();
  setName(name);
  setState(state);
  this->pin = pin;
  file.close();
  return true;
}
