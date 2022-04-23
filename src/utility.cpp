// служебные функции
#include <Arduino.h>
#include "def_soft.h" 

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

String padNum(int16_t num, byte cnt) { //сервисная функция вывода в строку
  char data[12];
  String fmt = "%0"+ String(cnt) + "d";
  sprintf(data, fmt.c_str(), num);
  return String(data);
}

String getDateTimeString(time_t t) { //вывод даты и времени в строку
  uint8_t hr = hour(t);
  uint8_t mn = minute(t);
  uint8_t sc = second(t);
  uint8_t dy = day(t);
  uint8_t mh = month(t);
  uint16_t yr = year(t);
  return padNum(dy,2) + "." + padNum(mh,2) + "." + padNum(yr,4) + " " + padNum(hr,2) + ":" + padNum(mn,2) + ":" + padNum(sc,2);  
}

String getTimeString(time_t t) {    //вывод времени в строку
  uint8_t hr = hour(t);
  uint8_t mn = minute(t);
  uint8_t sc = second(t);
  return padNum(hr,2) + ":" + padNum(mn,2) + ":" + padNum(sc,2);  
}

// leap year calulator expects year argument as years offset from 1970
boolean LEAP_YEAR(uint16_t Y) {
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
  Serial.print(t);
  Serial.print(F(" Текущее время: ")); 
  Serial.println(t2);

  setTime(t);

  #ifdef RTC
  rtc.adjust((DateTime)t); 
  Serial.print(F("RTC time sync: ")); 
  Serial.println(rtc.now().unixtime());
  #endif

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
#endif

#ifdef CO2CONTROL
    doc["minCO2"] = minCO2;
    doc["maxCO2"] = maxCO2;
    for (int i = 0; i < CO2_CYCLE; ++i)
    {
      String temps = (String)i + "CO2ON";
      if (CO2ON [i] > 0.0 && CO2ON [i] > 24.00)  doc[temps] = CO2ON [i];
             temps = (String)i + "CO2OFF";
      if (CO2OFF[i] > 0.0 && CO2OFF[i] > 24.00)  doc[temps] = CO2OFF[i];
    }
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
#endif
    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_PROF);

    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
  }
}

void calPointPub() {
  if (mqtt.connected()) {
    DynamicJsonDocument doc(256);
    String out;
#ifdef HUMCONTROL
#endif
#ifdef CO2CONTROL
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
#ifdef CO2CONTROL
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

boolean statusPub()    //Публикация состояния параметров системы
{
  if (mqtt.connected()) 
  {
    DynamicJsonDocument doc(256);
    String out;
    char s[8];   //строка mqtt сообщения
      doc["time"] = getTimeString(rtc.now().unixtime());
    #ifdef HUMCONTROL
      dtostrf(humd, 2, 2, s);
      doc["hum"] = humd;
      dtostrf(temp, 2, 2, s);
      doc["temp"] = temp;
      doc["Hum_relay"] = digitalRead(HUMPWR);
    #endif

    #ifdef CO2CONTROL
    // if(CO2PPM > 0) { 
      dtostrf(CO2PPM, 1, 2, s);
      doc["temp"] = temp;
      doc["CO2PPM"] = CO2PPM;
      doc["CO2_relay"] = digitalRead(CO2PWR);
    // }
    #endif

    #ifdef PHTDSCONTROL   
    if(Wtemp != DEVICE_DISCONNECTED_C && Wtemp > 0) { 
      dtostrf(Wtemp, 1, 2, s);
      switch (thisMode) { 
        case 0: doc["tSoil0"] = s; break;
        case 1: doc["tSoil1"] = s; break;
        case 2: doc["tSoil0"] = s; break;
      }
    }
    if (realPh != -1){
      dtostrf(realPh, 3, 3, s);
      switch (thisMode) { 
        case 0: doc["phSoil0"] = s; break;
        case 1: doc["phSoil1"] = s; break;
        case 2: doc["phSoil0"] = s; break;
      }
    }
    if ( realTDS  != -1 ) {
      dtostrf(realTDS, 1, 0, s);
      switch (thisMode) { 
        case 0: doc["tdsSoil0"] = s; break;
        case 1: doc["tdsSoil1"] = s; break;
        case 2: doc["tdsSoil0"] = s; break;
      }
    }

    if (thisMode != 0 && thisMode%2 != 0 ) doc["Wlvl"] = Wlvl;

    if(PhOk) doc["PhOk"] = 1; 
    else doc["PhOk"] = 0;

    doc["ColMd"] = thisMode;
#endif

    if(auto_mode) doc["auto"] = 1;
    else doc["auto"] = 0;

    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_HWSTAT);
    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
    return(true);
  }
  return(false);
}

#ifdef PHTDSCONTROL
boolean setCollector() //Приведение конфигурации коллектора в силу
{
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
}
#endif

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
    Serial.print(F("Подключение к "));
    Serial.print(ssid);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("connect to:");
    display.print(ssid);
    display.display();

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
    boolean stop_waiting = false;
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
          Serial.print(WiFi.macAddress());
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

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Wifi AP");
  display.display();

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

    display.setTextSize(1);
    display.setCursor(20, 50);
    display.print("udp:");
    display.print(localPort);
    display.display();

    Serial.print(F("UDP-сервер на порту "));
    Serial.print(localPort);
  }
}

#ifdef CO2CONTROL

bool CO2Control(int cur) // vkl/otkl CO2
{ 
  for (int i = 0; i < CO2_CYCLE; ++i)
    if (CO2ON[i] != 0 && CO2OFF[i] != 0) 
      if (TimeChk(CO2ON[i], CO2OFF[i]) == true) {
        CO2On = true;
        i = CO2_CYCLE;
      }

    if (CO2On == true && CO2Ready == true){
      if (cur <= minCO2) if(!digitalRead(CO2PWR)) digitalWrite(CO2PWR, HIGH);
      if (cur >= maxCO2) if( digitalRead(CO2PWR)) digitalWrite(CO2PWR, LOW ); 
    }
    else 
      if(digitalRead(CO2PWR)) digitalWrite(CO2PWR, LOW);

  return digitalRead(CO2PWR);   
}

int CO2Check (int check)
{
  for (int i = 0; i < CO2_CYCLE; ++i)
  {
    if (CO2ON[i] == check) return 1;
    if (CO2OFF[i] == check) return -1;
  }
  return 0;
}

bool TimeChk (int ON, int OFF)
{
  bool temps = false;
  if(ON != OFF) {
    int curtime = hour()*100 + minute();
    if (ON > OFF) {
      if (curtime >= ON  || curtime < OFF) if(temps != true) temps = true;
      if (curtime >= OFF && curtime < ON ) if(temps == true) temps = false;
    }
    else {
      if (curtime >= ON  && curtime < OFF) if(temps != true) temps = true;
      if (curtime >= OFF || curtime < ON ) if(temps == true) temps = false;
    }
  }
  else
    if(temps != true)  temps = true;
  
  return temps;
}


/*
// DM manualMode
void set_manualMode(boolean value) {
  if (manualMode == value) return;
  putAutoplay(value);
  manualMode = getAutoplay();
}

// PD autoplayTime
void set_autoplayTime(uint32_t value) {
  if (autoplayTime == value) return;
  putAutoplayTime(value);
  autoplayTime = getAutoplayTime();
}

// IT idleTime
void set_idleTime(uint32_t value) {
  if (idleTime == value) return;;
  putIdleTime(value);
  idleTime = getIdleTime();
}

// AL isAlarming 
void set_isAlarming(boolean value) {
  if (isAlarming == value) return;
  isAlarming = value;
}

// AL isAlarmStopped
void set_isAlarmStopped(boolean value) {
  if (isAlarmStopped == value) return;
  isAlarmStopped = value;
}
*/
void set_thisMode(int8_t value) {
  if (thisMode == value) return;
  
  // boolean valid = (value == -1) || (value >= 0 && value < MAX_EFFECT);
  // if (!valid) return;

  // valid = (value >= 0 && value < MAX_EFFECT);

  thisMode = value;
  putCurrentMode(thisMode);

#ifdef PHTDSCONTROL
  setCollector(); //Применение конфигурации коллектора
#endif
  if(thisMode%2 == 0 && thisMode != 0) count_mode = true;
  else count_mode = false;
  statusPub();
}

// useDHCP
void set_useDHCP(boolean value) {
  if (useDHCP == value) return;
  putUseDHCP(value);
  useDHCP = getUseDHCP();
}

// NP useNtp
void set_useNtp(boolean value) {
  if (useNtp == value) return;
  putUseNtp(value);
  useNtp = getUseNtp();
}

// NT syncTimePeriod
void set_syncTimePeriod(uint16_t value) {
  if (syncTimePeriod == value) return;
  putNtpSyncTime(value);
  syncTimePeriod = getNtpSyncTime();
}

// NZ timeZoneOffset
void set_timeZoneOffset(int16_t value) {
  if (timeZoneOffset == value) return;
  putTimeZone(value);
  timeZoneOffset = getTimeZone();
}

// NS ntpServerName
void set_ntpServerName(String value) {
  if (getNtpServer() == value) return;
  putNtpServer(value);  
  getNtpServer().toCharArray(ntpServerName, 31);
}

// NW ssid
void set_Ssid(String value) {
  if (getSsid() == value) return;
  putSsid(value);
  getSsid().toCharArray(ssid, 24);
}

// NA pass
void set_pass(String value) {
  if (getPass() == value) return;
  putPass(value);
  getPass().toCharArray(pass, 16);
}
              
// AN apName
void set_SoftAPName(String value) {
  if (getSoftAPName() == value) return;
  putSoftAPName(value);
  getSoftAPName().toCharArray(pass, 16);
}              

// AA apPass
void set_SoftAPPass(String value) {
  if (getSoftAPPass() == value) return;
  putSoftAPPass(value);
  getSoftAPPass().toCharArray(apPass, 16);
}              

// IP wifi_connected
void set_wifi_connected(boolean value) {
  if (wifi_connected == value) return;
  wifi_connected = value;
}              

// IP IP_STA[]
void set_StaticIP(byte p1, byte p2, byte p3, byte p4) {
  IP_STA[0] = p1; 
  IP_STA[1] = p2; 
  IP_STA[2] = p3; 
  IP_STA[3] = p4; 
  putStaticIP(p1, p2, p3, p4);
}              
/*
// AW alarmWeekDay
void set_alarmWeekDay(byte value) {
  if (alarmWeekDay == value) return;
  putAlarmParams(value,dawnDuration,alarmEffect,alarmDuration);
  alarmWeekDay = getAlarmWeekDay();
}

// AE alarmEffect
void set_alarmEffect(byte value) {
  if (alarmEffect == value) return;
  // byte alarmWeekDay = getAlarmWeekDay();
//  putAlarmParams(alarmWeekDay,dawnDuration,value,alarmDuration);
}

// MD alarmDuration
void set_alarmDuration(byte value) {
  if (alarmDuration == value) return;
  // byte alarmWeekDay = getAlarmWeekDay();
//  putAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,value);
}

// AT alarmHour[], alarmMinute[]
void set_alarmTime(byte wd, byte hour_value, byte minute_value) {
  byte old_hour   = getAlarmHour(wd);
  byte old_minute = getAlarmMinute(wd);
  if (old_hour == hour_value && old_minute == minute_value) return;
  putAlarmTime(wd, hour_value, minute_value);
  alarmHour[wd-1] = getAlarmHour(wd);
  alarmMinute[wd-1] = getAlarmMinute(wd);
}
*/
// AU useSoftAP
void set_useSoftAP(boolean value) {
  if (useSoftAP == value) return;
  putUseSoftAP(value);
  useSoftAP = getUseSoftAP();
}
/*
// AM1T AM1_hour
void set_AM1_hour(byte value) {
  if (AM1_hour == value) return;
  putAM1hour(value);
  AM1_hour = getAM1hour();
}

// AM1T AM1_minute
void set_AM1_minute(byte value) {
  if (AM1_minute == value) return;
  putAM1minute(value);
  AM1_minute = getAM1minute();
}

// AM1A AM1_effect_id
void set_AM1_effect_id(int8_t value) {
  if (AM1_effect_id == value) return;
  putAM1effect(value);
  AM1_effect_id = getAM1effect();  
}

// AM2T AM2_hour
void set_AM2_hour(byte value) {
  if (AM2_hour == value) return;
  putAM2hour(value);
  AM2_hour = getAM2hour();
}

// AM2T AM2_minute
void set_AM2_minute(byte value) {
  if (AM2_minute == value) return;
  putAM2minute(value);
  AM2_minute = getAM2minute();
}

// AM2A AM2_effect_id
void set_AM2_effect_id(int8_t value) {
  if (AM2_effect_id == value) return;
  putAM2effect(value);
  AM2_effect_id = getAM2effect();
}

// AM3T AM3_hour
void set_AM3_hour(byte value) {
  if (AM3_hour == value) return;
  putAM3hour(value);
  AM3_hour = getAM3hour();
}

// AM3T AM3_minute
void set_AM3_minute(byte value) {
  if (AM3_minute == value) return;
  putAM3minute(value);
  AM3_minute = getAM3minute();
}

// AM3A AM3_effect_id
void set_AM3_effect_id(int8_t value) {
  if (AM3_effect_id == value) return;
  putAM3effect(value);
  AM3_effect_id = getAM3effect();
}

// AM4T AM4_hour
void set_AM4_hour(byte value) {
  if (AM4_hour == value) return;
  putAM4hour(value);
  AM4_hour = getAM4hour();
}

// AM4T AM4_minute
void set_AM4_minute(byte value) {
  if (AM4_minute == value) return;
  putAM4minute(value);
  AM4_minute = getAM4minute();
}

// AM4A AM4_effect_id
void set_AM4_effect_id(int8_t value) {
  if (AM4_effect_id == value) return;
  putAM4effect(value);
  AM4_effect_id = getAM4effect();
}

// AM5A dawn_effect_id
void set_dawn_effect_id(int8_t value) {
  if (dawn_effect_id == value) return;
  putAM5effect(value);
  dawn_effect_id = getAM5effect();
}

// AM6A dusk_effect_id
void set_dusk_effect_id(int8_t value) {
  if (dusk_effect_id == value) return;
  putAM6effect(value);
  dusk_effect_id = getAM6effect();
}
*/

#if (USE_MQTT == 1)
// useMQTT
void set_useMQTT(boolean value) {
  if (useMQTT == value) return;  
  if (useMQTT || value) stopMQTT = false;
  putUseMqtt(value);
  useMQTT = getUseMqtt();
}

// mqtt_port
void set_mqtt_port(int16_t value) {
  if (mqtt_port == value) return;  
  putMqttPort(value);
  mqtt_port = getMqttPort();
}

// mqtt_server
void set_MqttServer(String value) {
  if (getMqttServer() == value) return;
  putMqttServer(value);
  getMqttServer().toCharArray(mqtt_server, 24);
}

// mqtt_user
void set_MqttUser(String value) {
  if (getMqttUser() == value) return;
  putMqttUser(value);
  getMqttUser().toCharArray(mqtt_user, 14);
}

// mqtt_pass
void set_MqttPass(String value) {
  if (getMqttPass() == value) return;
  putMqttPass(value);
  getMqttPass().toCharArray(mqtt_pass, 14);
}

// mqtt_send_delay
void set_mqtt_send_delay(int16_t value) {
  if (mqtt_send_delay == value) return;  
  putMqttSendDelay(value);
  mqtt_send_delay = getMqttSendDelay();
}

// mqtt_prefix
void set_MqttPrefix(String value) {
  if (getMqttPrefix() == value) return;
  putMqttPrefix(value);
  getMqttPrefix().toCharArray(mqtt_prefix, 30);
}

// mqtt_state_packet
void set_mqtt_state_packet(boolean value) {
  if (mqtt_state_packet == value) return;  
  putSendStateInPacket(value);
  mqtt_state_packet = getSendStateInPacket();
}

// upTimeSendInterval
void set_upTimeSendInterval(uint16_t value) {
  if (upTimeSendInterval == value) return;;
  putUpTimeSendInterval(value);
  upTimeSendInterval = getUpTimeSendInterval();
}
#endif

#endif