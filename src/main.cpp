#include <Arduino.h>
#include <def_soft.h>

#define FIRMWARE_VER F("WiFiMQTTNode-0.1")

uint16_t AUTO_MODE_PERIOD = 10;    // Период активации автоматического режима в минутах по умолчанию
uint16_t AUTO_FILL_PERIOD = 24;    // Период активации автоматического подлива в часах 
boolean  auto_mode = true;         // Флаг автоматического режима
boolean  count_mode = false;       // Флаг включения счетчика воды подлива

/* I2C адреса
OLED SSD1306  0x3C
RTC_DS3231    0x57
TDS           0x49
TDSadj        0x2E
PH            0x48
PHadj         0x2C
MotorEXT      0x20
WaterLVL      0x26  0x7C??
*/

#ifdef RTC
RTC_DS3231 rtc;
#endif

#ifdef AHTX0
Adafruit_AHTX0 aht;
sensors_event_t SensHum, SensTemp;
#endif

#ifdef CO2CONTROL
MHZ co2(MH_Z19_RX, MH_Z19_TX, MHZ19B);
int CO2Sel = 0, temp = 0, CO2PPM = 0;
boolean CO2Set = false, CO2On = false, CO2Ready = false;
uint16_t minCO2 = minCO2DEF, maxCO2 = maxCO2DEF;
int CO2ON [CO2_CYCLE] = {0,0,0};     //массив времен начала впрыска CO2
int CO2OFF[CO2_CYCLE] = {0,0,0};    //массив времен конца впрыска CO2
#endif

#ifdef HUMCONTROL
HTU21D myHumidity;
#endif

#ifdef PHTDSCONTROL
//Инициализация плат I2C расширителей
//Экзэмпляры классов

IoAbstractionRef ioExp2       = ioFrom8574  (0x24);          //Leds
IoAbstractionRef ioExpInp     = ioFrom8574  (0x26);        //Level Sensors
Adafruit_MCP23X17 mcp;        //Pumps
i2cPumps pumps(false);        //Pumps
boolean  booolik = true;      // Программная защелка

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#endif

// *************************** ПОДКЛЮЧЕНИЕ К СЕТИ **************************
WiFiUDP udp;                                // Объект транспорта сетевых пакетов
String  host_name;                          // Имя для регистрации в сети, а так же как имя клиента та сервере MQTT
char   apName[11] = DEFAULT_AP_NAME;        // Имя сети в режиме точки доступа
char   apPass[17] = DEFAULT_AP_PASS;        // Пароль подключения к точке доступа
char   ssid[25]   = NETWORK_SSID;           // SSID (имя) вашего роутера (конфигурируется подключением через точку доступа и сохранением в EEPROM)
char   pass[17]   = NETWORK_PASS;           // пароль роутера
byte   IP_STA[]   = DEFAULT_IP;             // Статический адрес в локальной сети WiFi по умолчанию при первом запуске. Потом - загружается из настроек, сохраненных в EEPROM
boolean   useDHCP    = USEDHCP;                // получать динамический IP
unsigned int localPort = 2390;              // локальный порт на котором слушаются входящие команды управления от приложения на смартфоне, передаваемые через локальную сеть

// --------------------Режимы работы Wifi соединения-----------------------
boolean   useSoftAP = false;               // использовать режим точки доступа
boolean   wifi_connected = false;          // true - подключение к wifi сети выполнена  
boolean   ap_connected = false;            // true - работаем в режиме точки доступа;

// **************** СИНХРОНИЗАЦИЯ ЧАСОВ ЧЕРЕЗ ИНТЕРНЕТ *******************

boolean   useNtp;                          // Использовать синхронизацию времени с NTP-сервером
IPAddress timeServerIP;                    // IP сервера времени
uint16_t  syncTimePeriod;                  // Период синхронизации в минутах по умолчанию
byte      packetBuffer[NTP_PACKET_SIZE];   // буфер для хранения входящих и исходящих пакетов NTP

int8_t timeZoneOffset;                  // смещение часового пояса от UTC
long   ntp_t   = 0;                     // Время, прошедшее с запроса данных с NTP-сервера (таймаут)
byte   ntp_cnt = 0;                     // Счетчик попыток получить данные от сервера
boolean   init_time = false;               // Флаг false - время не инициализировано; true - время инициализировано
boolean   refresh_time = true;             // Флаг true - пришло время выполнить синхронизацию часов с сервером NTP
boolean   getNtpInProgress = true;         // Запрос времени с NTP сервера в процессе выполнения
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
timerMinim AutoFillTimer (1000 * 3600 * AUTO_FILL_PERIOD);  // Таймер активации автоматического долива часов
//timerMinim AutoFillTimer (1000 * AUTO_FILL_PERIOD);  // Таймер активации автоматического долива секунд

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
  Serial.print("\nCommand from MQTT broker is : ");
  Serial.print("topic ");
  Serial.print(topic);
  Serial.print(" payload ");
  Serial.println(temp);
  #endif

  #ifdef HUMCONTROL
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
}
#endif

void setup() {
  #if defined(ESP8266)
    ESP.wdtEnable(WDTO_8S);
    Wire.begin();
  #endif
  #if defined(ESP32)
     #if defined(lolin32)//for lolin32 oled
        Wire.begin(5,4);
     #else
       Wire.begin(21,22);
     #endif
  #endif

  #ifdef HUMCONTROL                // Hum init
    myHumidity.begin();
  #endif

  #ifdef CO2CONTROL                // CO2 PPM MH-Z19B init
  #endif

  #ifdef AHTX0                     // AHT10 init
   if (! aht.begin()) Serial.println("Could not find AHT? Check wiring");
   else Serial.println("AHT10 or AHT20 found");
  #endif

#ifdef PHTDSCONTROL       
//Инициализация моторов (все выкл)
  mcp.begin_I2C(0x20);
  for(int i = 0; i < 16; i++ ){ 
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, pumps.getinit());
  }

 //init ioExp //test led
  for(int i = 0; i <= 7; i++ ){ 
    ioDevicePinMode     (ioExpInp,i, INPUT);
    ioDevicePinMode     (ioExp2,  i, OUTPUT);
    ioDeviceDigitalWrite(ioExp2,  i, true);
  }
  ioDeviceSync(ioExp2);
  ioDeviceSync(ioExpInp);
 #endif

  EEPROM.begin(EEPROM_MAX);

  Serial.begin(115200);
  while (!Serial);                    //ждем инициализации ком порта

  #ifdef RTC                       // RTC clock init
    if(rtc.begin())
    {
      Serial.print(F("\nRTC clock init: ")); 
      // if(!rtc.lostPower())
      { 
        setTime(rtc.now().unixtime());
        Serial.println((String)getDateTimeString(rtc.now().unixtime()));
      }
    }
  #endif

  host_name = String(HOST_NAME) + //"-" + 
  String(DEV_ID);
  Serial.print("FIRMWARE:\t");
  Serial.println(FIRMWARE_VER);
  Serial.println("Host name:\t" + host_name);

#ifdef PHTDSCONTROL
#ifdef DISPLAY

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) Serial.println(F("SSD1306 allocation failed")); 
#endif
#endif
  
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

  // if ((eeprom_backup & 0x02) > 0) {
  //   Serial.println(F("Найдены сохраненные настройки: SD://eeprom.bin"));
  // }
  loadSettings();

#ifdef PHTDSCONTROL
  setCollector(); //Применение конфигурации коллектора
#endif
#ifdef DISPLAY
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
#endif  

  // Подключение к сети
  connectToNetwork();

#ifdef PHTDSCONTROL
  setCollector(); //Применение конфигурации коллектора
#endif
#ifdef DISPLAY
  display.setTextSize(2);
  display.setCursor(0, 21);
  display.print("WARM. WAIT");
  display.display();
#endif

  ntpSyncTimer.setInterval ( 60000L * syncTimePeriod );

  #if (USE_MQTT == 1)
  // Настройка соединения с MQTT сервером

  stopMQTT = !useMQTT;
  changed_keys = "";
  last_mqtt_server = mqtt_server;
  last_mqtt_port = mqtt_port;
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(callback);
  checkMqttConnection();
  if (mqtt.connected())
  {
    String msg = F("START");
    SendMQTT(msg, TOPIC_STA);
  }

  #endif

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname(host_name.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {

#ifdef DISPLAY
    display.clearDisplay();
    display.setTextColor(WHITE);
#endif

    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = F("скетча...");
    else // U_SPIFFS
      type = F("файловой системы SPIFFS...");
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.print(F("Обновление начато"));    
    Serial.println(type);    
  });

  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nОбновление завершено"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("\nProgress: %u%%\r", (progress / (total / 100)));
#ifdef DISPLAY
    display.setTextSize(2);
    display.setCursor(0, 21);
    display.print("Flash-");
    display.print(progress / (total / 100));
    display.print("%");
    display.display();
#endif
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

#ifdef CO2CONTROL
  pinMode(CO2PWR, OUTPUT);
  if(rtc.lostPower()) timing1 = timing1 + 180000;
  else timing1 = timing1 + 60000;
#endif

#ifdef HUMCONTROL
  pinMode(HUMPWR, OUTPUT);
  // myHumidity.begin();
#endif

#ifdef PHTDSCONTROL
  //ds18b20 Begin "Dallas Temperature IC Control Library"
  sensors.begin();
  // if (!sensors.getAddress(WaterThermAdr, 0)) Serial.println("Unable to find address for WaterTherm"); 
  // // show the addresses we found on the bus
  // Serial.print("\nWater temp sensor Address: ");
  //   for (uint8_t i = 0; i < 8; i++) Serial.print(WaterThermAdr[i], HEX);
  // Serial.println();
  //sensors.setResolution(WaterThermAdr, 11);  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
//ds18b20 Begin end

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
  set_wifi_connected(WiFi.status() == WL_CONNECTED); 

  if (wifi_connected) {
    ArduinoOTA.handle();
    #if (USE_MQTT == 1)
      if (!stopMQTT) {
        checkMqttConnection();
        if (mqtt.connected()) mqtt.loop();
      }
    #endif
  }
  else {
    startWiFi(15000);
  }

  process();

  if(AutoModeTimer.isReady()){ //Активация автоматического режима
    auto_mode = true;
    profpub();
  }
  #if (USE_MQTT == 1)
  mqtt.loop();
  #endif
}