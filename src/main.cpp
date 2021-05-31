#include <Arduino.h>
//#include <def_hard.h>
#include <def_soft.h>

#define FIRMWARE_VER F("WiFiMQTTNode-0.1")

long lastReconnectAttempt = 0;
unsigned long timing, per;

float temp = 0, humd = 0, humcorr = 3.2,
tempcorr = 0.0;
float minhum, maxhum; // = minhumDEF // = maxhumDEF;

/*
void EEPROMWriteFloat(int addr, float val) // запись в ЕЕПРОМ
{
  byte *x = (byte *)&val;
  for(byte i = 0; i < 4; i++) EEPROM.write(i+addr, x[i]);
}

float EEPROMReadFloat(int addr) // чтение из ЕЕПРОМ
{
  byte x[4];
  for(byte i = 0; i < 4; i++) x[i] = EEPROM.read(i+addr);
  float *y = (float *)&x;
  return y[0];
}
*/
void profpub();
void loadSettings();

void setup_wifi();

void startWiFi(unsigned long waitTime);
void startSoftAP();
void connectToNetwork();
/*
#if (USE_MQTT == 1)

// ------------------ MQTT CALLBACK -------------------

void callback(char* topic, byte* payload, unsigned int length) {
  if (stopMQTT) return;
  // проверяем из нужного ли нам топика пришли данные
  Serial.print("MQTT << topic='" + String(topic) + "'");
  if (strcmp(topic, mqtt_topic(TOPIC_CMD).c_str()) == 0) {
    memset(incomeMqttBuffer, 0, BUF_MAX_SIZE);
    memcpy(incomeMqttBuffer, payload, length);
    
    Serial.print(F("; cmd='"));
    Serial.print(incomeMqttBuffer);
    Serial.print("'");
    
    // В одном сообщении может быть несколько команд. Каждая команда начинается с '$' и заканчивается ';'/ Пробелы между ';' и '$' НЕ допускаются.
    String command = String(incomeMqttBuffer);    
    command.replace("\n", "~");
    command.replace(";$", "\n");
    uint32_t count = CountTokens(command, '\n');
    
    for (uint8_t i=1; i<=count; i++) {
      String cmd = GetToken(command, i, '\n');
      cmd.replace('~', '\n');
      cmd.trim();
      // После разделения команд во 2 и далее строке '$' (начало команды) удален - восстановить
      if (!cmd.startsWith("$")) {
        cmd = "$" + cmd;
      }
      // После разделения команд во 2 и далее строке ';' (конец команды) удален - восстановить
      // Команда '$6 ' не может быть в пакете и признак ';' (конец команды) не используется - не восстанавливать
      if (!cmd.endsWith(";") && !cmd.startsWith(F("$6 "))) {
        cmd += ";";
      }        
      if (cmd.length() > 0 && queueLength < QSIZE_IN) {
        queueLength++;
        cmdQueue[queueWriteIdx++] = cmd;
        if (queueWriteIdx >= QSIZE_IN) queueWriteIdx = 0;
      }
    }    
  }
  Serial.println();
}

#endif


*/

void callback(char* topic, byte* payload, unsigned int length) 
{
//  
  Serial.print("Command from MQTT broker is : ");
  
  char temp[length];
  strncpy(temp, (char*)payload, length);
  //p = atoi(temp);
  Serial.print("topic ");
  Serial.print(topic);
  Serial.print(" payload ");
  Serial.println(temp);

  if ((String)topic == (String)mqtt_topic_max_hum)
  {
    maxhum = atof(temp);
    putMaxHum(maxhum);
    Serial.print('\n');
    Serial.print("putMaxHum ");
    Serial.print(maxhum);
    Serial.print(" - ");
    Serial.println(getMaxHum());

//    EEPROMWriteFloat(6, maxhum);
//    EEPROM.commit();
    profpub();
   }
  if ((String)topic == (String)mqtt_topic_min_hum)
  {
    minhum = atof(temp);
    putMinHum(minhum);
//    EEPROMWriteFloat(2,minhum);
//    EEPROM.commit();
    profpub();
   }

  if ((String)topic == (String)mqtt_topic_relay)
  {
    if ( atoi(temp) == 1 ) {
      digitalWrite (HUMPWR, HIGH);  
    } else {
      digitalWrite (HUMPWR, LOW);  
    }
    profpub();
  }
}

boolean reconnect() {
  Serial.print("Открываю MQTT соединение...");
  // Create a random client ID
  String clientId = mqttClient;
  clientId += String(random(0xffff), HEX);
//  if (client.connect(clientId.c_str(), mqtt_user, DEFAULT_MQTT_PASS))//mqtt_passwd))
  if (mqtt.connect(clientId.c_str(), DEFAULT_MQTT_USER, DEFAULT_MQTT_PASS))
  {
    Serial.println("подключено");
    mqtt.subscribe(mqtt_topic_min_hum);//
    mqtt.subscribe(mqtt_topic_max_hum);//
    mqtt.subscribe(mqtt_topic_relay);//
    mqtt.subscribe(mqtt_topic_com);//
  } else {
    Serial.print("неудача, статус соединения = ");
    Serial.println(mqtt.state());
  }
  return mqtt.connected();
}

void setup() {
  #if defined(ESP8266)
    ESP.wdtEnable(WDTO_8S);
  #endif

  // Инициализация EEPROM и загрузка начальных значений переменных и параметров
  // #if (EEPROM_MAX <= EEPROM_TEXT)
  //   #pragma message "Не выделено памяти для хранения строк эффекта 'Бегущая строка'"
  //   EEPROM.begin(EEPROM_TEXT);
  // #else  
  //    EEPROM.begin(EEPROM_MAX);
  // #endif

  EEPROM.begin(EEPROM_MAX);

  Serial.begin(115200);
  delay(300);

  host_name = String(HOST_NAME) + "-" + String(DEVICE_ID);

  Serial.println();
  Serial.println(FIRMWARE_VER);
  Serial.println("Host: '" + host_name + "'" + String(F(" >> ")));// + String(WIDTH) + "x" + String(HEIGHT));
  Serial.println();


//-------------------------Инициализация файловой системы--------------------

  Serial.println(F("\nИнициализация файловой системы... "));
  
  spiffs_ok = LittleFS.begin();
  if (!spiffs_ok) {
    Serial.println(F("\nВыполняется разметка файловой системы... "));
    LittleFS.format();
    spiffs_ok = LittleFS.begin();    
  }

  if (spiffs_ok) {
    Serial.print(F("FS: "));
    #if defined(ESP32)
      spiffs_total_bytes = LittleFS.totalBytes();
      spiffs_used_bytes  = LittleFS.usedBytes();
      Serial.println(String(F("Использовано ")) + String(spiffs_used_bytes) + " из " + String(spiffs_total_bytes) + " байт");
    #else
      FSInfo fs_info;
      if (LittleFS.info(fs_info)) {
        spiffs_total_bytes = fs_info.totalBytes;
        spiffs_used_bytes  = fs_info.usedBytes;
        Serial.println(String(F("Использовано ")) + String(spiffs_used_bytes) + " из " + String(spiffs_total_bytes) + " байт");
      } else {
        Serial.println(F("Ошибка получения сведений о файловой системе."));
      }
    #endif
  } else {
    Serial.println(F("Файловая система недоступна."));
  }

  // Проверить наличие резервной копии настроек EEPROM в файловой системе MK и/или на SD-карте
  eeprom_backup = checkEepromBackup();
  if ((eeprom_backup & 0x01) > 0) {
    Serial.println(F("Найдены сохраненные настройки: FS://eeprom.bin"));
  }
  if ((eeprom_backup & 0x02) > 0) {
    Serial.println(F("Найдены сохраненные настройки: SD://eeprom.bin"));
  }
    


  
  loadSettings();
  
    Serial.print('\n');
    Serial.print("maxhum ");
    Serial.print(maxhum);
    Serial.print(" - ");
    Serial.println(getMaxHum());


  // Подключение к сети
  connectToNetwork();
  
  #if (USE_MQTT == 1)
  // Настройка соединения с MQTT сервером
  stopMQTT = !useMQTT;
  changed_keys = "";
  last_mqtt_server = mqtt_server;
  last_mqtt_port = mqtt_port;
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);
  checkMqttConnection();    
  String msg = F("START");
  SendMQTT(msg, TOPIC_STA);
  #endif

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(mqttClient);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = F("скетча...");
    else // U_SPIFFS
      type = F("файловой системы SPIFFS...");
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.print(F("Начато обновление "));    
    Serial.println(type);    
  });

  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nОбновление завершено"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print(F("Ошибка: "));
    Serial.println(error);
    if      (error == OTA_AUTH_ERROR)    Serial.println(F("Неверное имя/пароль сети"));
    else if (error == OTA_BEGIN_ERROR)   Serial.println(F("Не удалось запустить обновление"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Не удалось установить соединение"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Не удалось получить данные"));
    else if (error == OTA_END_ERROR)     Serial.println(F("Ошибка завершения сессии"));
  });

  ArduinoOTA.begin();
  // UDP-клиент на указанном порту
  udp.begin(localPort);

  reconnect();
  profpub();
  
  Serial.print("minhum-");
  Serial.print(minhum);
  Serial.print("\tmaxhum-");
  Serial.println(maxhum);
  
  Serial.println(getMaxHum());


  pinMode(HUMPWR, OUTPUT);
  timing = millis() + REFRESHTIME;

  myHumidity.begin();
  lastReconnectAttempt = 0;  
}

void loop() {
  if (wifi_connected) {
    ArduinoOTA.handle();
    #if (USE_MQTT == 1)
      if (!stopMQTT) {
        checkMqttConnection();
        if (mqtt.connected()) {
          mqtt.loop();
        }
      }
    #endif
  }
  process();


//  ArduinoOTA.handle();
  
  if (millis() - timing > REFRESHTIME){
    // if (!mqtt.connected()) {
    //   long now = millis();
    //   if (now - lastReconnectAttempt > 5000) {
    //     lastReconnectAttempt = now;
    //     if (reconnect()) {
    //       lastReconnectAttempt = 0;
    //     }
    //   }
    // } 

    Serial.print("Time:");
    Serial.print(millis());
  
    humd = myHumidity.readHumidity() + humcorr;
    temp = myHumidity.readTemperature() + tempcorr;
    if(humd < 998){

      if ( humd > maxhum ){
        digitalWrite (HUMPWR, LOW);  
      }
      if ( humd < minhum ){
        digitalWrite (HUMPWR, HIGH);  
      }

      Serial.print(" Temperature:");
      Serial.print(temp, 3);
      Serial.print("C");
      Serial.print(" Humidity:");
      Serial.print(humd, 3);
      Serial.print("%");
      Serial.print(" UserRegister :");
      Serial.print(myHumidity.readUserRegister(),HEX);
      
      if (mqtt.connected()) {
        char s[8];
        dtostrf(humd, 2, 2, s);
        mqtt.publish(mqtt_topic_hum, s);

        dtostrf(temp, 2, 2, s);
        mqtt.publish(mqtt_topic_temp, s);
        
        profpub();
/*
        if (digitalRead(HUMPWR) == true) 
          mqtt.publish(mqtt_topic_hum_on, "1");
        else 
          mqtt.publish(mqtt_topic_hum_on, "0");
*/
      }
    }
    Serial.print("\n");
    timing = millis();
  }
  mqtt.loop();
}

void profpub() {
  if (mqtt.connected()) {
    DynamicJsonDocument doc(256);
    String out;
    doc["minhum"] = minhum;
    doc["maxhum"] = maxhum;
    doc["hum_relay"] = digitalRead(HUMPWR);
    serializeJson(doc, out);      
    SendMQTT(out, TOPIC_STT);

    bool ok = mqtt.beginPublish(mqtt_topic_stat, out.length(), false);
    // bool ok = mqtt.beginPublish(mqtt_topic_stat, load.length(), false);
    if(ok){
      mqtt.print(out.c_str());
      // mqtt.print(load.c_str());
      ok = mqtt.endPublish() == 1;
    }   
    if (ok) {      
      // Отправка прошла успешно
      Serial.print(F("MQTT >> OK >> ")); 
//      Serial.print(topic);
      Serial.print(mqtt_topic_stat);
      Serial.print(F("\t >> ")); 
      Serial.println(out);
      // Serial.println(load);
    } 
    else {
      // Отправка не удалась
      Serial.print(F("MQTT >> FAIL >> ")); 
//      Serial.print(topic);
      Serial.print(mqtt_topic_stat);
      Serial.print(F("\t >> ")); 
//      Serial.println(message);
      Serial.println(out);
      // Serial.println(load);

    }

//*/
    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
  }

}

void loadSettings() {

  // Адреса в EEPROM:
  //    0 - если EEPROM_OK - EEPROM инициализировано, если другое значение - нет                             // EEPROMread(0)                 // EEPROMWrite(0, EEPROM_OK)

  //    5 - использовать синхронизацию времени через NTP                                                     // getUseNtp()                   // putUseNtp(useNtp)
  //  6,7 - период синхронизации NTP (int16_t - 2 байта) в минутах                                           // getNtpSyncTime()              // putNtpSyncTime(SYNC_TIME_PERIOD)
  //    8 - time zone UTC+X                                                                                  // getTimeZone();                // putTimeZone(timeZoneOffset)

  //   10 - IP[0]                                                                                            // getStaticIP()                 // putStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3])
  //   11 - IP[1]                                                                                            // - " -                         // - " -
  //   12 - IP[2]                                                                                            // - " -                         // - " -
  //   13 - IP[3]                                                                                            // - " -                         // - " -
  //   14 - Использовать режим точки доступа                                                                 // getUseSoftAP()                // putUseSoftAP(useSoftAP)
  
  //   15 - maxhum                                                                                           // getMaxHum()                   // putMaxHum(maxhum)
  //   19 - minhum                                                                                           // getMinHum()                   // putMinHum(minhum)
  //   23 - 

  
  //  54-63   - имя точки доступа    - 10 байт                                                               // getSoftAPName().toCharArray(apName, 10)       // putSoftAPName(String(apName))       // char apName[11] = ""
  //  64-79   - пароль точки доступа - 16 байт                                                               // getSoftAPPass().toCharArray(apPass, 17)       // putSoftAPPass(String(apPass))       // char apPass[17] = "" 
  //  80-103  - имя сети  WiFi       - 24 байта                                                              // getSsid().toCharArray(ssid, 25)               // putSsid(String(ssid))               // char ssid[25]   = ""
  //  104-119 - пароль сети  WiFi    - 16 байт                                                               // getPass().toCharArray(pass, 17)               // putPass(String(pass))               // char pass[17]   = ""
  //  120-149 - имя NTP сервера      - 30 байт                                                               // getNtpServer().toCharArray(ntpServerName, 31) // putNtpServer(String(ntpServerName)) // char ntpServerName[31] = ""
 
  //  161 - Режим 3 по времени - часы                                                                        // getAM3hour()                   // putAM3hour(AM3_hour)
  //  162 - Режим 3 по времени - минуты                                                                      // getAM3minute()                 // putAM3minute(AM3_minute) 
  //  163 - Режим 3 по времени - так же как для режима 1                                                     // getAM3effect()                 // putAM3effect(AM3_effect_id)
  //  164 - Режим 4 по времени - часы                                                                        // getAM4hour()                   // putAM4hour(AM4_hour)
  //  165 - Режим 4 по времени - минуты                                                                      // getAM4minute()                 // putAM4minute(AM4_minute)
  //  166 - Режим 4 по времени - так же как для режима 1                                                     // getAM4effect()                 // putAM4effect(AM4_effect_id)

  // 182-206 - MQTT сервер (24 симв)                                                                         // getMqttServer().toCharArray(mqtt_server, 24)  // putMqttServer(String(mqtt_server))       // char mqtt_server[25] = ""
  // 207-221 - MQTT user (14 симв)                                                                           // getMqttUser().toCharArray(mqtt_user, 14)      // putMqttUser(String(mqtt_user))           // char mqtt_user[15] = ""
  // 222-236 - MQTT pwd (14 симв)                                                                            // getMqttPass().toCharArray(mqtt_pass, 14)      // putMqttPass(String(mqtt_pass))           // char mqtt_pass[15] = ""
  // 237,238 - MQTT порт                                                                                     // getMqttPort()                  // putMqttPort(mqtt_port)
  // 239 - использовать MQTT канал управления: 0 - нет 1 - да                                                // getUseMqtt()                   // putUseMqtt(useMQTT)  

  // 241,242 - задержка отпракии запросов MQTT серверу                                                       // getMqttSendDelay()             // putMqttSendDelay(mqtt_send_delay)

  // 249 - отправка параметров состояния в MQTT 0 - индивидуально, 1 - пакетами                              // getSendStateInPacket()         // putSendStateInPacket(mqtt_state_packet)  
  // 250-279 - префикс топика сообщения (30 симв)                                                            // getMqttPrefix()                // putMqttPrefix(mqtt_prefix)
  // 280,281 - интервал отправки значения uptime на MQTT-сервер                                              // getUpTimeSendInterval()        // putUpTimeSendInterval(upTimeSendInterval)
  //**282 - не используется
  //  ...
  //**299 - не используется

  // Сначала инициализируем имя сети/точки доступа, пароли и имя NTP-сервера значениями по умолчанию.
  // Ниже, если EEPROM уже инициализирован - из него будут загружены актуальные значения
  strcpy(apName, DEFAULT_AP_NAME);
  strcpy(apPass, DEFAULT_AP_PASS);
  strcpy(ssid, NETWORK_SSID);
  strcpy(pass, NETWORK_PASS);
  strcpy(ntpServerName, DEFAULT_NTP_SERVER);    

  #if (USE_MQTT == 1)
  strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
  strcpy(mqtt_user,   DEFAULT_MQTT_USER);
  strcpy(mqtt_pass,   DEFAULT_MQTT_PASS);
  strcpy(mqtt_prefix, DEFAULT_MQTT_PREFIX);
  #endif

  // Инициализировано ли EEPROM
  bool isInitialized = EEPROMread(0) == EEPROM_OK;  
  
  Serial.println(isInitialized);

  if (isInitialized) {    
    maxhum = getMaxHum();
    minhum = getMinHum();
  
    Serial.print('\n');
    Serial.print("maxhum -i ");
    Serial.print(maxhum);
    Serial.print(" - ");
    Serial.println(getMaxHum());

    useNtp = getUseNtp();
    timeZoneOffset = getTimeZone();

    SYNC_TIME_PERIOD = getNtpSyncTime();
    useSoftAP = getUseSoftAP();
    getSoftAPName().toCharArray(apName, 10);        //  54-63   - имя точки доступа    ( 9 байт макс) + 1 байт '\0'
    getSoftAPPass().toCharArray(apPass, 17);        //  64-79   - пароль точки доступа (16 байт макс) + 1 байт '\0'
    getSsid().toCharArray(ssid, 25);                //  80-103  - имя сети  WiFi       (24 байта макс) + 1 байт '\0'
    getPass().toCharArray(pass, 17);                //  104-119 - пароль сети  WiFi    (16 байт макс) + 1 байт '\0'
    getNtpServer().toCharArray(ntpServerName, 31);  //  120-149 - имя NTP сервера      (30 байт макс) + 1 байт '\0'
    
    if (strlen(apName) == 0) strcpy(apName, DEFAULT_AP_NAME);
    if (strlen(apPass) == 0) strcpy(apPass, DEFAULT_AP_PASS);
    if (strlen(ntpServerName) == 0) strcpy(ntpServerName, DEFAULT_NTP_SERVER);

    #if (USE_MQTT == 1)
    useMQTT = getUseMqtt();
    mqtt_state_packet = getSendStateInPacket();
    getMqttServer().toCharArray(mqtt_server, 25);   //  182-206 - mqtt сервер          (24 байт макс) + 1 байт '\0'
    getMqttUser().toCharArray(mqtt_user, 15);       //  207-221 - mqtt user            (14 байт макс) + 1 байт '\0'
    getMqttPass().toCharArray(mqtt_pass, 15);       //  222-236 - mqtt password        (14 байт макс) + 1 байт '\0'
    getMqttPrefix().toCharArray(mqtt_prefix, 31);   //  250-279 - mqtt password        (30 байт макс) + 1 байт '\0'
    if (strlen(mqtt_server) == 0) strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
    if (strlen(mqtt_user)   == 0) strcpy(mqtt_user,   DEFAULT_MQTT_USER);
    if (strlen(mqtt_pass)   == 0) strcpy(mqtt_pass,   DEFAULT_MQTT_PASS);
    if (strlen(mqtt_prefix) == 0) strcpy(mqtt_prefix, DEFAULT_MQTT_PREFIX);
    mqtt_port = getMqttPort();
    mqtt_send_delay = getMqttSendDelay();
    upTimeSendInterval = getUpTimeSendInterval();
    #endif

    getStaticIP();
    
  } else {

    Serial.println(F("Инициализация EEPROM..."));

    // Значения переменных по умолчанию определяются в месте их объявления - в файле def_soft.h
    // Здесь выполняются только инициализация массивов и некоторых специальных параметров
    clearEEPROM();

    // После первой инициализации значений - сохранить их принудительно
    saveDefaults();
    saveSettings();
    
    maxhum = getMaxHum();
    minhum = getMinHum();

  }  

  #if (USE_MQTT == 1) 
  changed_keys = "";
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

    if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0) {
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
    WiFi.begin(ssid, pass);

    // Проверка соединения (таймаут 180 секунд, прерывается при необходимости нажатием кнопки)
    // Такой таймаут нужен в случае, когда отключают электричество, при последующем включении устройство стартует быстрее
    // чем роутер успеет загрузиться и создать сеть. При коротком таймауте устройство не найдет сеть и создаст точку доступа,
    // не сможет получить время, погоду и т.д.
    bool stop_waiting = false;
    unsigned long start_wifi_check = millis();
    unsigned long last_wifi_check = 0;
    int16_t cnt = 0;
    while (!(stop_waiting || wifi_connected)) {
      delay(0);
      if (millis() - last_wifi_check > 250) {
        last_wifi_check = millis();
        //set_wifi_connected(WiFi.status() == WL_CONNECTED); 
        if (WiFi.status() == WL_CONNECTED) wifi_connected = true;
        

        if (wifi_connected) {
          // Подключение установлено
          Serial.println();
          Serial.print(F("WiFi подключен. IP адрес: "));
          Serial.println(WiFi.localIP());
          break;
        }
        if (cnt % 50 == 0) {
          Serial.println();
        }
        Serial.print(".");
        cnt++;
      }
      if (millis() - start_wifi_check > waitTime) {
        // Время ожидания подключения к сети вышло
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
  startWiFi(180000);

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