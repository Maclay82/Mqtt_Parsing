// служебные функции
#include <Arduino.h>
//#include "def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

uint32_t HEXtoInt(String hexValue) {
  hexValue.toUpperCase();
  if (hexValue.charAt(0) == '#') {  hexValue = hexValue.substring(1);  }

  if (hexValue.startsWith("0X")) {  hexValue = hexValue.substring(2);  }

  byte tens, ones, number1, number2, number3;
  tens = (hexValue[0] <= '9') ? hexValue[0] - '0' : hexValue[0] - '7';
  ones = (hexValue[1] <= '9') ? hexValue[1] - '0' : hexValue[1] - '7';
  number1 = (16 * tens) + ones;

  tens = (hexValue[2] <= '9') ? hexValue[2] - '0' : hexValue[2] - '7';
  ones = (hexValue[3] <= '9') ? hexValue[3] - '0' : hexValue[3] - '7';
  number2 = (16 * tens) + ones;

  tens = (hexValue[4] <= '9') ? hexValue[4] - '0' : hexValue[4] - '7';
  ones = (hexValue[5] <= '9') ? hexValue[5] - '0' : hexValue[5] - '7';
  number3 = (16 * tens) + ones;

  return ((uint32_t)number1 << 16 | (uint32_t)number2 << 8 | number3 << 0);
}

// uint32_t to Hex string
String IntToHex(uint32_t value) {
  String sHex = "00000" + String(value, HEX);
  byte len = sHex.length();
  if (len > 6) {
    sHex = sHex.substring(len - 6);
    sHex.toUpperCase();
  }
  return sHex;
}

uint32_t CountTokens(String str, char separator) {

  uint32_t count = 0;
  int pos = 0;
  String l_str = str;

  l_str.trim();
  if (l_str.length() <= 0) return 0;
  pos = l_str.indexOf(separator);
  while (pos >= 0) {
    count++;
    pos = l_str.indexOf(separator, pos + 1);
  }
  return ++count;
}

String GetToken(String &str, uint32_t index, char separator) {

  uint32_t count = CountTokens(str, separator);

  if (count <= 1 || index < 1 || index > count) return str;

  uint32_t pos_start = 0;
  uint32_t pos_end = str.length();

  count = 0;
  for (uint32_t i = 0; i < pos_end; i++) {
    if (str.charAt(i) == separator) {
      count++;
      if (count == index) {
        pos_end = i;
        break;
      } else {
        pos_start = i + 1;
      }
    }
  }
  return str.substring(pos_start, pos_end);
}

String padNum(int16_t num, byte cnt) {
  char data[12];
  String fmt = "%0"+ String(cnt) + "d";
  sprintf(data, fmt.c_str(), num);
  return String(data);
}

String getDateTimeString(time_t t) {
  uint8_t hr = hour(t);
  uint8_t mn = minute(t);
  uint8_t sc = second(t);
  uint8_t dy = day(t);
  uint8_t mh = month(t);
  uint16_t yr = year(t);
  return padNum(dy,2) + "." + padNum(mh,2) + "." + padNum(yr,4) + " " + padNum(hr,2) + ":" + padNum(mn,2) + ":" + padNum(sc,2);  
}

// leap year calulator expects year argument as years offset from 1970
bool LEAP_YEAR(uint16_t Y) {
  return ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) );
}

void sendNTPpacket(IPAddress& address) {
  Serial.print(F("Отправка NTP пакета на сервер "));
  Serial.println(ntpServerName);
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write((const uint8_t*) packetBuffer, NTP_PACKET_SIZE);  
  udp.endPacket();
  udp.flush();
  delay(0);
}

void parseNTP() {
  getNtpInProgress = false;
  Serial.println(F("Разбор пакета NTP"));
  ntp_t = 0; ntp_cnt = 0; init_time = true; refresh_time = false;
  unsigned long highWord = word(incomeBuffer[40], incomeBuffer[41]);
  unsigned long lowWord = word(incomeBuffer[42], incomeBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;    
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  unsigned long seventyYears = 2208988800UL ;
  unsigned long t = secsSince1900 - seventyYears + (timeZoneOffset) * 3600UL;
  String t2 = getDateTimeString(t);

  Serial.print(F("Секунд с 1970: "));
  Serial.println(t);
  Serial.print(F("Текущее время: ")); 
  Serial.println(t2);

  setTime(t);  
  //calculateDawnTime();
  //rescanTextEvents();

  // Если время запуска еще не определено - инициализировать его
  if (upTime == 0) {
    upTime = t - millis() / 1000L;
  }

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(256);
  String out;
  doc["act"] = F("TIME");
  doc["server_name"] = ntpServerName;
  doc["server_ip"] = timeServerIP.toString();
  doc["result"] = F("OK");
  doc["time"] = secsSince1900;
  doc["s_time2"] = t2;
  serializeJson(doc, out);      
  SendMQTT(out, TOPIC_TME);
  #endif
}

void getNTP() {
  if (!wifi_connected) return;
  WiFi.hostByName(ntpServerName, timeServerIP);
  IPAddress ip1, ip2;
  ip1.fromString(F("0.0.0.0"));
  ip2.fromString(F("255.255.255.255"));
  if (timeServerIP == ip1 || timeServerIP == ip2) {
    Serial.print(F("Не удалось получить IP aдрес сервера NTP -> "));
    Serial.print(ntpServerName);
    Serial.print(F(" -> "));
    Serial.println(timeServerIP);
    timeServerIP.fromString(F("85.21.78.91"));  // Один из ru.pool.ntp.org  // 91.207.136.55, 91.207.136.50, 46.17.46.226
    Serial.print(F("Используем сервер по умолчанию: "));
    Serial.println(timeServerIP);
  }
  getNtpInProgress = true;
  //printNtpServerName();  
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  ntp_t = millis();  

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(256);
  String out;
  doc["act"] = F("TIME");
  doc["server_name"] = ntpServerName;
  doc["server_ip"] = timeServerIP.toString();
  doc["result"] = F("REQUEST");
  serializeJson(doc, out);
  SendMQTT(out, TOPIC_TME);
  #endif
}


void profpub() {
  if (mqtt.connected()) {
    DynamicJsonDocument doc(256);
    String out;

#ifdef HUMCONTROL
    doc["minhum"] = minhum;
    doc["maxhum"] = maxhum;
    doc["hum_relay"] = digitalRead(HUMPWR);
#endif

#ifdef PHTDSCONTROL
    doc["regDelay"] = regDelay / 60000;
    doc["phmax"] = phmax;
    doc["phmin"] = phmin;
    doc["tdsmax"] = tdsmax;
    doc["tdsmin"] = tdsmin;
    doc["phVol"] = phVol;
    doc["tdsAVol"] = tdsAVol;
    doc["tdsBVol"] = tdsBVol;
    doc["tdsCVol"] = tdsCVol;
    doc["mode"] = thisMode;
#endif

    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_PROF);

    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
  }
}

void CalprofPub() {
  if (mqtt.connected()) {
    DynamicJsonDocument doc(256);
    String out;
#ifdef HUMCONTROL
#endif

#ifdef PHTDSCONTROL
    doc["PhCP1"] = PhCalP1;
    doc["rPhCP1"] = rawPhCalP1;
    doc["PhCP2"] = PhCalP2;
    doc["rPhCP2"] = rawPhCalP2;
    doc["TDSCP1"] = TDSCalP1;
    doc["rDSCP1"] = rawTDSCalP1;        
    doc["TDSCP2"] = TDSCalP2;        
    doc["rTDSCP2"] = rawTDSCalP2;
    doc["P1Sc"] = getPumpScl(1);
    doc["P2Sc"] = getPumpScl(2);
    doc["P3Sc"] = getPumpScl(3);
    doc["P4Sc"] = getPumpScl(4);
    doc["P5Sc"] = getPumpScl(5);
    doc["P6Sc"] = getPumpScl(6);
    doc["P7Sc"] = getPumpScl(7);
    doc["P8Sc"] = getPumpScl(8);
#endif
    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_CAL);
    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
  }
}

void HWprofPub() {
  if (mqtt.connected()) {
    DynamicJsonDocument doc(256);
    String out;
#ifdef HUMCONTROL

#endif
#ifdef PHTDSCONTROL
    doc["RAWMode"] = RAWMode; //getTDSKb средняя точка
    doc["tKb"] = tdsKb; //getTDSKb средняя точка
    doc["tKa"] = tdsKa; //getTDSKa усиление
    doc["pKb"] = phKb; //getPhKb средняя точка
    doc["pKa"] = phKa; //getPhKa усиление
#endif

    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_HWSET);
    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
  }
}

bool statusPub()    //Публикация состояния параметров системы
{
  if (mqtt.connected()) 
  {
    DynamicJsonDocument doc(256);
    String out;
    char s[8];   //строка mqtt сообщения

#ifdef HUMCONTROL

#endif

#ifdef PHTDSCONTROL   
    if(Wtemp != DEVICE_DISCONNECTED_C && Wtemp > 0) { 
      dtostrf(Wtemp, 2, 2, s);
      switch (thisMode) { 
        case 0: doc["tempSoil0"] = s; break;
        case 1: doc["tempSoil1"] = s; break;
        case 2: doc["tempSoil0"] = s; break;
      }
    }
    if (realPh != -1){
      dtostrf(realPh, 2, 3, s);
      switch (thisMode) { 
        case 0: doc["phSoil0"] = s; break;
        case 1: doc["phSoil1"] = s; break;
        case 2: doc["phSoil0"] = s; break;
      }
    }
    if ( realTDS  != -1 ) {
      dtostrf(realTDS, 2, 0, s);
      switch (thisMode) { 
        case 0: doc["tdsSoil0"] = s; break;
        case 1: doc["tdsSoil1"] = s; break;
        case 2: doc["tdsSoil0"] = s; break;
      }
    }

    doc["PhOk"] = PhOk;
    doc["mode"] = thisMode;
#endif

    if(auto_mode) doc["auto_mode"] = 1;
    else doc["auto_mode"] = 0;

    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_HWSTAT);
    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
    return(true);
  }
  return(false);
}

bool setCollector() //Приведение конфигурации коллектора в силу
{
#ifdef HUMCONTROL

#endif
#ifdef PHTDSCONTROL
  if(ioDeviceSync(ioExp2) == true)
  {
    switch (thisMode) 
    { 
      case 0:
        ioDeviceDigitalWrite(ioExp2, ClWaterIn, HIGH);
        ioDeviceDigitalWrite(ioExp2, ClWaterOut, HIGH);
        ioDeviceDigitalWrite(ioExp2, SolWaterIn_1, LOW);
        ioDeviceDigitalWrite(ioExp2, SolWaterOut_1, LOW);
      break;
      case 1:
        ioDeviceDigitalWrite(ioExp2, ClWaterIn, LOW);
        ioDeviceDigitalWrite(ioExp2, ClWaterOut, LOW);
        ioDeviceDigitalWrite(ioExp2, SolWaterIn_1, HIGH);
        ioDeviceDigitalWrite(ioExp2, SolWaterOut_1, HIGH);
      break;
      case 2:
        ioDeviceDigitalWrite(ioExp2, ClWaterIn, HIGH);
        ioDeviceDigitalWrite(ioExp2, ClWaterOut, LOW);
        ioDeviceDigitalWrite(ioExp2, SolWaterIn_1, LOW);
        ioDeviceDigitalWrite(ioExp2, SolWaterOut_1, HIGH);
      break;
    }
  }          
  return ioDeviceSync(ioExp2);
#endif
}

void startWiFi(unsigned long waitTime) { 
  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
  #endif  

  // We start by connecting to a WiFi network
  WiFi.disconnect(true);
  set_wifi_connected(false);
  
  delay(10);               // Иначе получаем Core 1 panic'ed (Cache disabled but cached memory region accessed)
  WiFi.mode(WIFI_STA);
  // Пытаемся соединиться с роутером в сети
  if (strlen(ssid) > 0) {
    Serial.print(F("\nПодключение к "));
    Serial.print(ssid);

    if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0 && useDHCP == false) {
      WiFi.config(IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]),  // 192.168.0.106 
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // 192.168.0.1
                  IPAddress(255, 255, 255, 0),                            // Mask
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // DNS1 192.168.0.1
                  IPAddress(8, 8, 8, 8));                                 // DNS2 8.8.8.8    
      Serial.print(F(" -> "));
      Serial.print(IP_STA[0]);
      Serial.print(".");
      Serial.print(IP_STA[1]);
      Serial.print(".");
      Serial.print(IP_STA[2]);
      Serial.print(".");
      Serial.print(IP_STA[3]);                  
    }
   
    WiFi.setHostname (host_name.c_str());             

    WiFi.begin(ssid, pass);

    // Проверка соединения (таймаут 180 секунд, прерывается при необходимости нажатием кнопки)
    // Такой таймаут нужен в случае, когда отключают электричество, при последующем включении устройство стартует быстрее
    // чем роутер успеет загрузиться и создать сеть. При коротком таймауте устройство не найдет сеть и создаст точку доступа,
    // не сможет получить время и т.д.
    bool stop_waiting = false;
    unsigned long start_wifi_check = millis();
    unsigned long last_wifi_check = 0;
    int16_t cnt = 0;
    while (!(stop_waiting || wifi_connected)) {
      delay(0);
      if (millis() - last_wifi_check > 250) {
        last_wifi_check = millis();
        set_wifi_connected(WiFi.status() == WL_CONNECTED); 
        if (WiFi.status() == WL_CONNECTED) wifi_connected = true;
        
        if (wifi_connected) {
          // Подключение установлено
          Serial.print(F("\nWiFi подключен. IP адрес: "));
          Serial.print(WiFi.localIP());
          Serial.print(F(" MAC адрес: "));
          Serial.println(WiFi.macAddress());
          break;
        }
        if (cnt % 80 == 0) {
          Serial.println();
        }
        Serial.print(".");
        cnt++;
      }
      if (millis() - start_wifi_check > waitTime) {
        // Время ожидания подключения к сети вышло
        Serial.print("\nВремя ожидания подключения к сети вышло");
        break;
      }
      delay(0);
      // Опрос состояния кнопки
      // butt.tick();
      // if (butt.hasClicks()) {
      //   butt.getClicks();
      //   Serial.println();
      //   Serial.println(F("Нажата кнопка.\nОжидание подключения к сети WiFi прервано."));  
      //   stop_waiting = true;
      //   break;
      // }
      //delay(0);
    }
    Serial.println();

    if (!wifi_connected && !stop_waiting)
      Serial.println(F("Не удалось подключиться к сети WiFi."));
  }  
}

void startSoftAP() {
  WiFi.softAPdisconnect(true);
  ap_connected = false;

  Serial.print(F("Создание точки доступа "));
  Serial.print(apName);
  if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0 && useDHCP == false) {
    WiFi.config(IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]),  // 192.168.0.106
                IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // 192.168.0.1
                IPAddress(255, 255, 255, 0),                            // Mask
                IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // DNS1 192.168.0.1
                IPAddress(8, 8, 8, 8));                                 // DNS2 8.8.8.8                  
    Serial.print(F(" -> "));
    Serial.print(IP_STA[0]);
    Serial.print(".");
    Serial.print(IP_STA[1]);
    Serial.print(".");
    Serial.print(IP_STA[2]);
    Serial.print(".");
    Serial.print(IP_STA[3]);                  
  }              
  
  ap_connected = WiFi.softAP(apName, apPass);

  for (int j = 0; j < 10; j++ ) {    
    delay(0);
    if (ap_connected) {
      Serial.println();
      Serial.print(F("Точка доступа создана. Сеть: '"));
      Serial.print(apName);
      // Если пароль совпадает с паролем по умолчанию - печатать для информации,
      // если был изменен пользователем - не печатать
      if (strcmp(apPass, "12341234") == 0) {
        Serial.print(F("'. Пароль: '"));
        Serial.print(apPass);
      }
      Serial.println(F("'."));
      Serial.print(F("IP адрес: "));
      Serial.println(WiFi.softAPIP());
      break;
    }    
    
    WiFi.enableAP(false);
    WiFi.softAPdisconnect(true);
    delay(500);
    
    Serial.print(".");
    ap_connected = WiFi.softAP(apName, apPass);
  }  
  Serial.println();  

  if (!ap_connected) 
    Serial.println(F("Не удалось создать WiFi точку доступа."));
}

void connectToNetwork() {  // Подключиться к WiFi сети, ожидать подключения 180 сек пока, например, после отключения электричества роутер загрузится и поднимет сеть
  startWiFi(300000);

  // Если режим точки доступа не используется и к WiFi сети подключиться не удалось - создать точку доступа
  if (!wifi_connected){
    WiFi.mode(WIFI_AP);
    startSoftAP();
  }

  if (useSoftAP && !ap_connected) startSoftAP();    

  // Сообщить UDP порт, на который ожидаются подключения
  if (wifi_connected || ap_connected) {
    Serial.print(F("UDP-сервер на порту "));
    Serial.println(localPort);
  }
}