#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>              // Библиотека для работы с файловой системой
#include <ESP8266FtpServer.h>// Библиотека для работы с SPIFFS по FTP
#include "CTBot.h"
#include <EEPROM.h>
#include "configuration.h"
#include "relay.h"
#include "PIR.h"
#include <PubSubClient.h>

#define MAX_SRV_CLIENTS 5
#define host "smartSwitch" //mdns имя

const char* mqtt_server = "192.168.1.2";
const char* www_username = "admin"; //логин и пароль для веб авторизации
const char* www_password = "admin";
const char* mqttUser = "mqtt_client";
const char*mqttPassword = "2s2chx6ZwvBM2Bg";
const char* pirSensorTopic = "/sensor1";
const char* relayTopic = "/switch1";

#define relay_pin 0
#define pir_pin 3
Configuration conf;
Relay relay0;
PirSensor pir3;

ESP8266WebServer server(80);//веб сервер
FtpServer ftpSrv;  //ftp сервер
CTBot telegramBot;
int64_t accessList[10];
byte tailsAccessList = 0;
String accessPassword = "!qwer$";

WiFiClient serverClients[MAX_SRV_CLIENTS];
PubSubClient mqttClient(serverClients[1]);

bool handleFileRead(String path) { // Функция работы веб сервера с файловой системой
  if (path.endsWith("/"))
    path += "index.html";         // Если устройство вызывается по корневому адресу, то должен вызываться файл index.html (добавляем его в конец адреса)
  String contentType = getContentType(path);// Определяем по типу файла (в адресе обращения) какой заголовок необходимо возвращать по его вызову
  if (SPIFFS.exists(path)) {     // Если в файловой системе существует файл по адресу обращения
    File file = SPIFFS.open(path, "r");     //  Открываем файл для чтения
    server.streamFile(file, contentType);   //  Выводим содержимое файла по HTTP, указывая заголовок типа содержимого contentType
    file.close();                 //  Закрываем файл
    return true;                  //  Завершаем выполнение функции, возвращая результатом ее исполнения true (истина)
  }
  return false;                   // Завершаем выполнение функции, возвращая результатом ее исполнения false (если не обработалось предыдущее условие)
}

String getContentType(String filename) {     // Функция, возвращающая необходимый заголовок типа содержимого в зависимости от расширения файла
  if (filename.endsWith(".html")) return "text/html";                   // Если файл заканчивается на ".html", то возвращаем заголовок "text/html" и завершаем выполнение функции
  else if (filename.endsWith(".css")) return "text/css";                // Если файл заканчивается на ".css", то возвращаем заголовок "text/css" и завершаем выполнение функции
  else if (filename.endsWith(".js")) return "application/javascript";   // Если файл заканчивается на ".js", то возвращаем заголовок "application/javascript" и завершаем выполнение функции
  else if (filename.endsWith(".png")) return "image/png";               // Если файл заканчивается на ".png", то возвращаем заголовок "image/png" и завершаем выполнение функции
  else if (filename.endsWith(".jpg")) return "image/jpeg";              // Если файл заканчивается на ".jpg", то возвращаем заголовок "image/jpg" и завершаем выполнение функции
  else if (filename.endsWith(".gif")) return "image/gif";               // Если файл заканчивается на ".gif", то возвращаем заголовок "image/gif" и завершаем выполнение функции
  else if (filename.endsWith(".ico")) return "image/x-icon";            // Если файл заканчивается на ".ico", то возвращаем заголовок "image/x-icon" и завершаем выполнение функции
  return "text/plain";                                                  // Если ни один из типов файла не совпал, то считаем что содержимое файла текстовое, отдаем соответствующий заголовок и завершаем выполнение функции
}

void sendSensorDataToMqttBroker(){
  if (mqttClient.connected()) {
    String message = String(pir3.getState());
    mqttClient.publish(pirSensorTopic, message.c_str());
    }
}

void sendRelayDataToMqttBroker(bool relayState){
  if (mqttClient.connected()) {
    String message = String(relayState);
    mqttClient.publish(relayTopic, message.c_str());
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
//  if (strcmp(topic, relayTopic) == 0) {
    if ((char)payload[0] == '1') {
      relay0.setState(true);
    }
    else if ((char)payload[0] == '0') {
      relay0.setState(false);
    }
//  }
}

bool mqttConnected(byte interval){
  static unsigned int last = 0;
  if (!mqttClient.connected()){
     if ((millis()-last)>interval) {
        if (mqttClient.connect("ESP8266Client", mqttUser, mqttPassword)) {
            mqttClient.subscribe(relayTopic);
            return true;
        }
      last=millis();
     }
      return false;
  }
  return true;
}

bool clientConnected(int Seconds) { //Подключение к известной точке доступа WiFi, если удалось подключение в течении заданного времени возвращаем True, иначе False
  WiFi.begin(conf.wifi.get_ssid().c_str(), conf.wifi.get_pass().c_str());
  for (int i = 0; i < Seconds; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    if (WiFi.status() == WL_CONNECTED){
     return true;
    }
  }
  return false;
}

String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;
  bool negative = 0;
  if (input < 0) {
    negative = 1;
    input *= -1;
  }
  do {
    char c = input % base;
    input /= base;
    c += '0';
    result = c + result;
  } while (input);
  return (negative) ? ('-' + result) : result;
}

void setWifiSettings() { //задание параметров подключения к сети
  if (server.hasArg("SSID") && server.hasArg("PASSAP")) {
    if ((server.arg("SSID") != NULL) && (server.arg("PASSAP") != NULL)) {
      String header = "HTTP/1.1 301 OK\r\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      String web_ssid = server.arg("SSID");
      String web_pass = server.arg("PASSAP");
      conf.wifi.set_credentials(web_ssid, web_pass); //перевод строки в массив символов в стиле C
      if (clientConnected(20)) //попытка подключения к заданной сети
        conf.save(); //если удалось подключиться, сохраняем параметры
    }
  }
}

void setToken() { //задание параметров подключения к сети
  if (server.hasArg("token") && (server.arg("token") != NULL)) {
    String header = "HTTP/1.1 301 OK\r\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    String web_token = server.arg("token");
    conf.set_token(web_token);
    conf.save(); //если удалось подключиться, сохраняем параметры
  }
}

String eepromdump()
{ //снимаем дамп данных из EEPROM
  EEPROM.begin(100);
  byte tmp = 0;
  String result = "";
  for (byte i = 0; i < 100; i++)
  {
    EEPROM.get(i, tmp);
    result += String(tmp) + "|";
  }
  EEPROM.end();
  return result;
}

void syncSensor(unsigned int intervalMilSec){
  static unsigned int lastUpdate=0;
  if(millis()-lastUpdate>intervalMilSec){
    if (pir3.syncState())
      sendSensorDataToMqttBroker();   
    lastUpdate=millis();
  }
}

void setup(void) {
  delay(1000);
  //выставляем режимы работы необходимых пинов  
  conf.load(); //загружаем параметры из памяти
  relay0.init(relay_pin);
  sendRelayDataToMqttBroker(relay0.getState()); 
  pir3.init(pir_pin);
  WiFi.mode(WIFI_STA);//преход в режим клиента
  conf.wifi.set_mode(true);
  
  if (!clientConnected(20)) { //если не удается подключиться к точке доступа, переходим в режим AP
    WiFi.mode(WIFI_AP);
    conf.wifi.set_mode(false);
    WiFi.softAP("smartSwitch", "test1234"); //ssid password
    digitalWrite(LED_BUILTIN, LOW);
  }
  else { //если удалось подключиться к известной сети
    digitalWrite(LED_BUILTIN, LOW);
  }
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
  //запросы веб сервера
  server.on("/", []() {
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    handleFileRead("/");
    server.send(200, "text/plain", "Login success");
  });
  //задание параметров подключения к сети
  server.on("/wifi", HTTP_POST, []() {
    server.send(200, "text/plain", "wifi");
    setWifiSettings();
  });

  //дамп памяти
  server.on("/mem_dump", HTTP_GET, []() {
    server.send(200, "text/plain", eepromdump());
  });

  //обновление прошивки через веб сервер
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    int uperror = Update.hasError();
    if (uperror == 0) server.send(200, "text/html", "Firmware update successfully <a href='/'>BACK</a>");
    else server.send(200, "text/html", "Update error <a href='/'>BACK</a>");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      WiFiUDP::stopAll();
      if (upload.filename == NULL) {
        server.send(200, "text/html", "<html> zero file size <a href='/upload'>BACK</a></html>");
        return (-1);
      }
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
      } else {
      }
    }
  //  yield();
  return 0;
  });

  //перезагрузка устройства
  server.on("/reboot", [] {
    server.send(200, "text/html", "successfully <a href='/'>BACK</a>");
    ESP.restart();
  });
  
  //управление реле
  server.on("/relay", HTTP_GET, []() {
    String message="";
    byte n=0;
    if (server.hasArg("get")){
      message= "?name1="+relay0.getName();
    message+="&state1="+String(relay0.getState());
  } else if (server.hasArg("set")){           
              if (server.hasArg("name"))
                relay0.setName(server.arg("name"));
              if (server.hasArg("state")){
                relay0.setState(String(server.arg("state")).equals("1"));
                sendRelayDataToMqttBroker(relay0.getState());
              }
            message="Success";            
         } else if (server.hasArg("save")){
                relay0.save();
                message="Success";
                }
                else if (server.hasArg("load")){
                      relay0.load();
                      message="Success";
                      }
  server.send(200, "text/plane", String(message));
  });

//управление сенсором
  server.on("/sensor", HTTP_GET, []() {
    String message="";
    byte n=0;
    if (server.hasArg("get")){
      message= "?name1="+pir3.getName();
    message+="&state1="+String(pir3.getState());
  } else if (server.hasArg("set")){           
              if (server.hasArg("name"))
                pir3.setName(server.arg("name"));
                message="Success";            
         } else if (server.hasArg("save")){
                pir3.save();
                message="Success";
                }
                else if (server.hasArg("load")){
                      pir3.load();
                      message="Success";
                      }
  server.send(200, "text/plane", String(message));
  });

  server.onNotFound([]() {                                                // Описываем действия при событии "Не найдено"
    if (!handleFileRead(server.uri()))                                      // Если функция handleFileRead (описана ниже) возвращает значение false в ответ на поиск файла в файловой системе
      server.send(404, "text/plain", "Not Found");                        // возвращаем на запрос текстовое сообщение "File isn't found" с кодом 404 (не найдено)
  });

  MDNS.addService("http", "tcp", 80);
  SPIFFS.begin();
  MDNS.begin(host);
  server.begin();
  ftpSrv.begin("admin", "admin");
  //relay0.load();
  //pir3.load();
}

void loop(void) {
  server.handleClient();
  ftpSrv.handleFTP();
  mqttClient.loop();
  mqttConnected(10000);
  syncSensor(1000);
  delay(100);
  if (conf.wifi.get_mode()) {//true - client, если в режиме клиента, проверяем подключение
    if (WiFi.status() != WL_CONNECTED)
      clientConnected(30);
    else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
