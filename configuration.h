class Configuration
{
  public:
    class Wifi
    {
      public:
        Wifi();
        Wifi(bool, String, String);
        void set_credentials(String, String);
        String get_ssid(void);
        String get_pass(void);
        void set_mode(bool);
        bool get_mode();
      private:
        String _ssid, _pass;
        bool _mode; //переменная хранит текущий режим работы wifi-> true - client, false-AP
    };
    Wifi wifi;
    Configuration();
    void save(void);
    void load(void);
    String get_token(void);
    void set_token(String);
  private:
    void putStringToEEPROM(byte, String);
    String getStringFromEEPROM(byte,byte);
    String _token;

};

void Configuration::load()
{ //загрузка параметров из памяти EEPROM
  String s_mem_ssid, s_mem_pass, s_mem_token;
  EEPROM.begin(100); //с запасом , только читаем
  byte lengthSsid, lengthPass,lengthToken;
  EEPROM.get(0, lengthSsid); //считываем длинну строки ssid
  EEPROM.get(1 + lengthSsid, lengthPass); //считываем строку ssid
  EEPROM.get(2 + lengthSsid + lengthPass, lengthToken);
  if (lengthSsid > 60 or lengthPass > 60) //если длинна строк больше максимума завершаем процедуру
      return;
  char ok[2 + 1];
  EEPROM.get(3 + lengthSsid + lengthPass + lengthToken, ok); //считываем маркер
  EEPROM.end();

  s_mem_ssid=getStringFromEEPROM(1,lengthSsid);
  s_mem_pass=getStringFromEEPROM(2+lengthSsid,lengthPass);
  s_mem_token=getStringFromEEPROM(3+lengthSsid + lengthPass,lengthToken);
  
    if (String(ok) == String("OK")) //если маркер верный, применяем параметры
  {
    wifi.set_credentials(s_mem_ssid, s_mem_pass);
    _token=s_mem_token;
  }
}

void Configuration::save()
{ //сохранение текущих параметров в EEPROM
  String s_ssid = wifi.get_ssid();
  String s_pass = wifi.get_pass();
  byte lengthSsid = byte(s_ssid.length()); //длинна строки в байтах
  byte lengthPass = byte(s_pass.length());
  byte lengthToken = byte(_token.length());
    
  putStringToEEPROM(1, s_ssid); //записываем строку ssid
  putStringToEEPROM(2 + lengthSsid, s_pass); //записываем строку пароль
  putStringToEEPROM(3 + lengthSsid + lengthPass, _token);

  char ok[2 + 1] = "OK"; //маркер
  EEPROM.begin(lengthSsid + lengthPass + lengthToken+3+3);//3 строки 3 байта с длиннами строк + маркер
  EEPROM.put(3 + lengthSsid + lengthPass + lengthToken, ok); // записываем маркер
  EEPROM.end();
}

void Configuration::putStringToEEPROM(byte startAddr, String data) {
  //записывает строку в EEPROM startAddr - адрес начала записи, data -строка для записи
  byte dataLength = byte(data.length());
  EEPROM.begin(1+startAddr + dataLength);
  byte k = 0; //счетчик символов в строке
  EEPROM[startAddr-1] = dataLength;
  for (byte i = startAddr; i < dataLength + startAddr; i++)
  {
    EEPROM[i] = data[k];
    k++;
    delay(10);
  }
  EEPROM.end();
}

String Configuration::getStringFromEEPROM(byte startAddr, byte strLength){
  String result="";
  EEPROM.begin(startAddr+strLength+100);
  for (byte i = startAddr; i < strLength + startAddr; i++) //по байтно считываем строку из EEPROM
  {
    result += (char)EEPROM[i];
    delay(10);
    }
  EEPROM.end();
  return result;
}

void Configuration::set_token(String token){
  _token=token;
}

String Configuration::get_token(void){
  return _token;
}

Configuration::Configuration()
{
  wifi = Wifi();
}

void Configuration::Wifi::set_mode(bool mode_)
{
  _mode = mode_;
}

bool Configuration::Wifi::get_mode(void)
{
  return _mode;
}

Configuration::Wifi::Wifi()
{
  _mode = false;
  _ssid = "";
  _pass = "";
}

Configuration::Wifi::Wifi(bool mode_, String ssid, String pass)
{
  _mode = mode_;
  _ssid = ssid;
  _pass = pass;
}

String Configuration::Wifi::get_ssid()
{
  return _ssid;
}

String Configuration::Wifi::get_pass()
{
  return _pass;
}

void Configuration::Wifi::set_credentials(String ssid, String pass)
{
  _ssid = ssid;
  _pass = pass;
}
