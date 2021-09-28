#include <Arduino.h>
#include <def_soft.h>

#define FIRMWARE_VER F("WiFiMQTTNode-0.1")

uint16_t AUTO_MODE_PERIOD = 10;    // Период активации автоматического режима в минутах по умолчанию
bool     auto_mode = true;         // Флаг автоматического режима
bool     count_mode = false;       // Флаг включения счетчика воды подлива

#ifdef PHTDSCONTROL
//Инициализация плат I2C расширителей
//Экзэмпляры классов
i2cPumps pumps(0x20, true);                       //Pumps
IoAbstractionRef ioExp2   = ioFrom8574(0x24);     //Leds
IoAbstractionRef ioExpInp = ioFrom8574(0x26);     //Level Sensors
#endif

// *************************** ПОДКЛЮЧЕНИЕ К СЕТИ **************************
WiFiUDP udp;                                // Объект транспорта сетевых пакетов
String  host_name;                          // Имя для регистрации в сети, а так же как имя клиента та сервере MQTT
char   apName[11] = DEFAULT_AP_NAME;        // Имя сети в режиме точки доступа
char   apPass[17] = DEFAULT_AP_PASS;        // Пароль подключения к точке доступа
char   ssid[25]   = NETWORK_SSID;           // SSID (имя) вашего роутера (конфигурируется подключением через точку доступа и сохранением в EEPROM)
char   pass[17]   = NETWORK_PASS;           // пароль роутера
byte   IP_STA[]   = DEFAULT_IP;             // Статический адрес в локальной сети WiFi по умолчанию при первом запуске. Потом - загружается из настроек, сохраненных в EEPROM
bool   useDHCP    = USEDHCP;                // получать динамический IP
unsigned int localPort = 2390;              // локальный порт на котором слушаются входящие команды управления от приложения на смартфоне, передаваемые через локальную сеть

// --------------------Режимы работы Wifi соединения-----------------------
bool   useSoftAP = false;               // использовать режим точки доступа
bool   wifi_connected = false;          // true - подключение к wifi сети выполнена  
bool   ap_connected = false;            // true - работаем в режиме точки доступа;

// **************** СИНХРОНИЗАЦИЯ ЧАСОВ ЧЕРЕЗ ИНТЕРНЕТ *******************

bool      useNtp;                       // Использовать синхронизацию времени с NTP-сервером
IPAddress timeServerIP;                 // IP сервера времени
uint16_t  syncTimePeriod;               // Период синхронизации в минутах по умолчанию
byte      packetBuffer[NTP_PACKET_SIZE];// буфер для хранения входящих и исходящих пакетов NTP

int8_t timeZoneOffset;                  // смещение часового пояса от UTC
long   ntp_t   = 0;                     // Время, прошедшее с запроса данных с NTP-сервера (таймаут)
byte   ntp_cnt = 0;                     // Счетчик попыток получить данные от сервера
bool   init_time = false;               // Флаг false - время не инициализировано; true - время инициализировано
bool   refresh_time = true;             // Флаг true - пришло время выполнить синхронизацию часов с сервером NTP
bool   getNtpInProgress = true;         // Запрос времени с NTP сервера в процессе выполнения
char   ntpServerName[31] = "";          // Используемый сервер NTP

uint32_t upTime = 0;                    // время работы системы с последней перезагрузки

#if (USE_MQTT == 1)
#define    BUF_MQTT_SIZE  1024//384     // максимальный размер выделяемого буфера для входящих сообщений по MQTT каналу
char incomeMqttBuffer[BUF_MQTT_SIZE];   // Буфер для приема строки команды из MQTT
#endif
// ************************* ПРОЧИЕ ПЕРЕМЕННЫЕ *************************

// ---------------------------------------------------------------

// Сервер не может инициировать отправку сообщения клиенту - только в ответ на запрос клиента
// Следующие две переменные хранят сообщения, формируемые по инициативе сервера и отправляются в ответ на ближайший запрос от клиента,
// например в ответ на периодический ping - в команде sendAcknowledge();

String   cmd95;                        // Строка, формируемая sendPageParams(95) для отправки по инициативе сервера
String   cmd96;                        // Строка, формируемая sendPageParams(96) для отправки по инициативе сервера

// ---------------------------------------------------------------

int8_t   thisMode = 1;                 // текущий режим - id
String   effect_name;                  // текущий режим - название

// ---------------------------------------------------------------
//timerMinim idleTimer(idleTime);             // Таймер бездействия ручного управления для автоперехода в демо-режим 

timerMinim saveSettingsTimer(15000);                      // Таймер отложенного сохранения настроек
timerMinim ntpSyncTimer  (1000 * 60 * syncTimePeriod);  // Сверяем время с NTP-сервером через syncTimePeriod минут
timerMinim AutoModeTimer (1000 * 60 * AUTO_MODE_PERIOD);  // Таймер активации автоматического режима через AUTO_MODE_PERIOD минут

#if (USE_MQTT == 1)

// ------------------ MQTT CALLBACK -------------------

void callback(char* topic, byte* payload, unsigned int length) {
  if (stopMQTT) return;
  // проверяем из нужного ли нам топика пришли данные
#ifdef USE_LOG
  Serial.print("MQTT << topic='" + String(topic) + "'");
#endif
  if (strcmp(topic, mqtt_topic(TOPIC_CMD).c_str()) == 0) {
    memset(incomeMqttBuffer, 0, BUF_MAX_SIZE);
    memcpy(incomeMqttBuffer, payload, length);
    
#ifdef USE_LOG
    Serial.print(F("; cmd='"));
    Serial.print(incomeMqttBuffer);
    Serial.print("'");
#endif
    
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

  char temp[length];
  strncpy(temp, (char*)payload, length);

#ifdef USE_LOG
  Serial.println();
  Serial.print("Command from MQTT broker is : ");
  Serial.print("topic ");
  Serial.print(topic);
  Serial.print(" payload ");
  Serial.println(temp);
#endif

#ifdef HUMCONTROL
  if ((String)topic == (String)mqtt_topic(TOPIC_MAXHUM).c_str())
  {
    maxhum = atof(temp);
    putMaxHum(maxhum);
    profpub();
  }
  if ((String)topic == (String)mqtt_topic(TOPIC_MINHUM).c_str())
  {
    minhum = atof(temp);
    putMinHum(minhum);
    profpub();
  }

  if ((String)topic == (String)mqtt_topic(TOPIC_RELAY).c_str())//(String)mqtt_topic_relay)
  {
    if ( atoi(temp) == 1 ) {
      digitalWrite (HUMPWR, HIGH);
      AutoModeTimer.reset();  
      auto_mode = false;
      profpub();
    } else {
      digitalWrite (HUMPWR, LOW);
      AutoModeTimer.reset();  
      auto_mode = false;
      profpub();
    }
  }
#endif

#ifdef PHTDSCONTROL
  if ((String)topic == (String)TOPIC_phKa){
    phKa = atoi(temp);
    Wire.beginTransmission(PHREGADR); // transmit to device
    Wire.write(byte(0x01));            // sends instruction byte  
    Wire.write(phKa);             // sends potentiometer value byte  
    Wire.endTransmission();     // stop transmitting

    Serial.print(" phKa:");
    Serial.print(phKa);
    Serial.println();
    putPhKa  (phKa);
    profpub();
  }
  if ((String)topic == (String)TOPIC_phKb){
    phKb = atoi(temp);
    Wire.beginTransmission(PHREGADR); // transmit to device
    Wire.write(byte(0x02));            // sends instruction byte  
    Wire.write(phKb);             // sends potentiometer value byte  
    Wire.endTransmission();     // stop transmitting

    Serial.print(" phKb:");
    Serial.print(phKb);
    Serial.println();
    putPhKb  (phKb);
    profpub();
  }
  if ((String)topic == (String)TOPIC_tdsKa){
    tdsKa = atoi(temp);
    Wire.beginTransmission(TDSREGADR); // transmit to device
    Wire.write(byte(0x01));            // sends instruction byte  
    Wire.write(tdsKa);             // sends potentiometer value byte  
    Wire.endTransmission();     // stop transmitting

    Serial.print(" tdsKa:");
    Serial.print(tdsKa);
    Serial.println();
    putTDSKa (tdsKa);  // усиление
    profpub();
  }
  if ((String)topic == (String)TOPIC_tdsKb){
    tdsKb = atoi(temp);
    Wire.beginTransmission(TDSREGADR); // transmit to device
    Wire.write(byte(0x02));            // sends instruction byte  
    Wire.write(tdsKb);             // sends potentiometer value byte  
    Wire.endTransmission();     // stop transmitting

    Serial.print(" tdsKb:");
    Serial.print(tdsKb);
    putTDSKb (tdsKb);
    profpub();
  }
#endif
}

#endif

void setup() {
  #if defined(ESP8266)
    ESP.wdtEnable(WDTO_8S);
  #endif
  
  Wire.begin();

#ifdef PHTDSCONTROL

  for(int i = 0; i <= 7; i++ ){ 
    //ioDevicePinMode(ioExp, i, OUTPUT);
    ioDevicePinMode(ioExp2, i, OUTPUT);
    ioDevicePinMode(ioExpInp, i, INPUT);
  }

  for(int i = 0; i <= 7; i++ ){
    //ioDeviceDigitalWrite(ioExp, i, !true);
    ioDeviceDigitalWrite(ioExp2, i, true);
  }
  ioDeviceSync(ioExp2);
  ioDeviceSync(ioExpInp);
#endif

  EEPROM.begin(EEPROM_MAX);

  Serial.begin(115200);
  delay(300);

  host_name = String(HOST_NAME) + //"-" + 
  String(DEV_ID);
  Serial.println();
  Serial.println(FIRMWARE_VER);
  Serial.println("Host: '" + host_name + "'");//String(HOST_NAME) + "'");
  
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

  setCollector(); //Приведение конфигурации коллектора в силу
  
  // Подключение к сети
  connectToNetwork();

  ntpSyncTimer.setInterval ( 1000L * 60 * syncTimePeriod );

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
  SendMQTT(msg, TOPIC_MQTTSTT);
  #endif

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(host_name.c_str());

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

//  reconnect();
  profpub();

  timing = timing1 = timing2 = millis();
  timing3 = timing2 + ( regDelay / 2 );

#ifdef HUMCONTROL
  pinMode(HUMPWR, OUTPUT);
  myHumidity.begin();
#endif

#ifdef PHTDSCONTROL

  phk = ( PhCalP2 - PhCalP1 ) / ( rawPhCalP2 - rawPhCalP1 );
  PhMP = phk * rawPhCalP1 - PhCalP1;
  tdsk = ( TDSCalP2 - TDSCalP1 ) / ( rawTDSCalP2 - rawTDSCalP1 );
  TdsMP = tdsk * rawTDSCalP1 - TDSCalP1;

  Wire.beginTransmission(PHREGADR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));            // sends instruction byte  
  Wire.write(phKa);             // sends potentiometer value byte  
  Wire.write(byte(0x02));            // sends instruction byte  
  Wire.write(phKb);             // sends potentiometer value byte  
  Wire.endTransmission();     // stop transmitting

  delay(200);

  Wire.beginTransmission(TDSREGADR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));            // sends instruction byte  
  Wire.write(tdsKa);             // sends potentiometer value byte  
  Wire.write(byte(0x02));            // sends instruction byte  
  Wire.write(tdsKb);             // sends potentiometer value byte  
  Wire.endTransmission();     // stop transmitting
#endif
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

  if(AutoModeTimer.isReady()){ //Активация автоматического режима
    auto_mode = true;
    profpub();
  }

  mqtt.loop();
}