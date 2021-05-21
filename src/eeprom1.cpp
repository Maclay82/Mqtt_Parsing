#include <Arduino.h>
#include "a_def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
#include "a_def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.
#include "eeprom1.h"
#include "utility.h"

void loadSettings() {

  // Адреса в EEPROM:
  //    0 - если EEPROM_OK - EEPROM инициализировано, если другое значение - нет                             // EEPROMread(0)                 // EEPROMWrite(0, EEPROM_OK)

  //    2 - автосмена режима в демо: вкл/выкл                                                                // getAutoplay();                // putAutoplay(manualMode)
  //    3 - время автосмены режимов в сек                                                                    // getAutoplayTime()             // putAutoplayTime(autoplayTime / 1000L)     // autoplayTime - мс; в ячейке - в сек
  //    4 - время бездействия до переключения в авторежим в минутах                                          // getIdleTime()                 // putIdleTime(idleTime / 60L / 1000L)       // idleTime - мс; в ячейке - в мин
  //    5 - использовать синхронизацию времени через NTP                                                     // getUseNtp()                   // putUseNtp(useNtp)
  //  6,7 - период синхронизации NTP (int16_t - 2 байта) в минутах                                           // getNtpSyncTime()              // putNtpSyncTime(SYNC_TIME_PERIOD)
  //    8 - time zone UTC+X                                                                                  // getTimeZone();                // putTimeZone(timeZoneOffset)

  //   10 - IP[0]                                                                                            // getStaticIP()                 // putStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3])
  //   11 - IP[1]                                                                                            // - " -                         // - " -
  //   12 - IP[2]                                                                                            // - " -                         // - " -
  //   13 - IP[3]                                                                                            // - " -                         // - " -
  //   14 - Использовать режим точки доступа                                                                 // getUseSoftAP()                // putUseSoftAP(useSoftAP)

  //   20 - Будильник, дни недели                                                                            // getAlarmWeekDay()             // putAlarmParams(alarmWeekDay, dawnDuration, alarmEffect, alarmDuration)

  //   29 - Номер последнего активированного вручную режима                                                  // getCurrentManualMode()        // putCurrentManualMode(xxx)

  //   33 - Режим 1 по времени - часы                                                                        // getAM1hour()                  // putAM1hour(AM1_hour)
  //   34 - Режим 1 по времени - минуты                                                                      // getAM1minute()                // putAM1minute(AM1_minute) 
  //   35 - Режим 1 по времени - -3 - выкл. (не исп.); -2 - выкл. (черный экран); -1 - ночн.часы, 0 - случ., // getAM1effect()                // putAM1effect(AM1_effect_id)
  //   36 - Режим 2 по времени - часы                                      ^^^ 1,2..N - эффект EFFECT_LIST  // getAM2hour()                  // putAM2hour(AM2_hour) 
  //   37 - Режим 2 по времени - минуты                                                                      // getAM2minute()                // putAM2minute(AM2_minute)
  //   38 - Режим 2 по времени - = " = как для режима 1                                                      // getAM2effect()                // putAM2effect(AM2_effect_id)

  //   40 - Будильник, время: понедельник : часы                                                             // getAlarmHour(1)               // putAlarmTime(1, alarmHour[0], alarmMinute[0])  // for (byte i=0; i<7; i++) alarmHour[i] = getAlarmHour(i+1)
  //   41 - Будильник, время: понедельник : минуты                                                           // getAlarmMinute(1)             // putAlarmTime(1, alarmHour[0], alarmMinute[0])  // for (byte i=0; i<7; i++) alarmMinute[i] = getAlarmMinute(i+1)
  //   42 - Будильник, время: вторник : часы                                                                 // getAlarmHour(2)               // putAlarmTime(2, alarmHour[1], alarmMinute[1])  // for (byte i=0; i<7; i++) putAlarmTime(i+1, alarmHour[i], alarmMinute[i])
  //   43 - Будильник, время: вторник : минуты                                                               // getAlarmMinute(2)             // putAlarmTime(2, alarmHour[1], alarmMinute[1])
  //   44 - Будильник, время: среда : часы                                                                   // getAlarmHour(3)               // putAlarmTime(3, alarmHour[2], alarmMinute[2])
  //   45 - Будильник, время: среда : минуты                                                                 // getAlarmMinute(3)             // putAlarmTime(3, alarmHour[2], alarmMinute[2])
  //   46 - Будильник, время: четверг : часы                                                                 // getAlarmHour(4)               // putAlarmTime(4, alarmHour[3], alarmMinute[3])
  //   47 - Будильник, время: четверг : минуты                                                               // getAlarmMinute(4)             // putAlarmTime(4, alarmHour[3], alarmMinute[3])
  //   48 - Будильник, время: пятница : часы                                                                 // getAlarmHour(5)               // putAlarmTime(5, alarmHour[4], alarmMinute[4])
  //   49 - Будильник, время: пятница : минуты                                                               // getAlarmMinute(5)             // putAlarmTime(5, alarmHour[4], alarmMinute[4])
  //   50 - Будильник, время: суббота : часы                                                                 // getAlarmHour(6)               // putAlarmTime(6, alarmHour[5], alarmMinute[5])
  //   51 - Будильник, время: суббота : минуты                                                               // getAlarmMinute(6)             // putAlarmTime(6, alarmHour[5], alarmMinute[5])
  //   52 - Будильник, время: воскресенье : часы                                                             // getAlarmHour(7)               // putAlarmTime(7, alarmHour[6], alarmMinute[6])
  //   53 - Будильник, время: воскресенье : минуты                                                           // getAlarmMinute(7)             // putAlarmTime(7, alarmHour[6], alarmMinute[6])
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

  // 244,245,246,247 - Код региона OpenWeatherMap для получения погоды (4 байта - uint32_t)                  // getWeatherRegion2()            // putWeatherRegion2(regionID2)

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
  
  if (isInitialized) {    
    autoplayTime = getAutoplayTime();
    idleTime = getIdleTime();

    useNtp = getUseNtp();
    timeZoneOffset = getTimeZone();

    SYNC_TIME_PERIOD = getNtpSyncTime();
    manualMode = getAutoplay();
    
    alarmWeekDay = getAlarmWeekDay();

    // Загрузить недельные будильники / часы, минуты /
    for (byte i=0; i<7; i++) {
      alarmHour[i] = getAlarmHour(i+1);
      alarmMinute[i] = getAlarmMinute(i+1);
    }
 
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

    AM1_hour       = getAM1hour();
    AM1_minute     = getAM1minute();
    AM1_effect_id  = getAM1effect();
    AM2_hour       = getAM2hour();
    AM2_minute     = getAM2minute();
    AM2_effect_id  = getAM2effect();
    AM3_hour       = getAM3hour();
    AM3_minute     = getAM3minute();
    AM3_effect_id  = getAM3effect();
    AM4_hour       = getAM4hour();
    AM4_minute     = getAM4minute();
    AM4_effect_id  = getAM4effect();
    dawn_effect_id = getAM5effect();
    dusk_effect_id = getAM6effect();

//    loadTexts();
    getStaticIP();
    
  } else {

    Serial.println(F("Инициализация EEPROM..."));

    // Значения переменных по умолчанию определяются в месте их объявления - в файле a_def_soft.h
    // Здесь выполняются только инициализация массивов и некоторых специальных параметров
    clearEEPROM();

    // Значения текстовых строк по умолчанию - 
    textLines[0] = "##";
    for (byte i = 1; i<36; i++) textLines[i] = "";

    // После первой инициализации значений - сохранить их принудительно
    saveDefaults();
    saveSettings();
    
    Serial.println();
  }  

  #if (USE_MQTT == 1) 
  changed_keys = "";
  #endif
}

void clearEEPROM() {
  for (int addr = 1; addr < EEPROM_MAX; addr++) {
    EEPROM.write(addr, 0);
  }
}

void saveDefaults() {

  putAutoplayTime(autoplayTime / 1000L);
  putIdleTime(constrain(idleTime / 60L / 1000L, 0, 255));

  putUseNtp(useNtp);
  putTimeZone(timeZoneOffset);

//  putNtpSyncTime(SYNC_TIME_PERIOD);
  putAutoplay(manualMode);


  for (byte i=0; i<7; i++) {
    putAlarmTime(i+1, alarmHour[i], alarmMinute[i]);
  }

  putUseSoftAP(useSoftAP);

  strcpy(apName, DEFAULT_AP_NAME);
  strcpy(apPass, DEFAULT_AP_PASS);
  strcpy(ssid, NETWORK_SSID);
  strcpy(pass, NETWORK_PASS);

  #if (USE_MQTT == 1)
  strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
  strcpy(mqtt_user, DEFAULT_MQTT_USER);
  strcpy(mqtt_pass, DEFAULT_MQTT_PASS);
  #endif  

  putSoftAPName(String(apName));
  putSoftAPPass(String(apPass));
  putSsid(String(ssid));
  putPass(String(pass));

  #if (USE_MQTT == 1)
  putMqttServer(String(mqtt_server));
  putMqttUser(String(mqtt_user));
  putMqttPass(String(mqtt_pass));
  putMqttPrefix(String(mqtt_prefix));
  putMqttPort(mqtt_port);
  putUseMqtt(useMQTT);
  putSendStateInPacket(mqtt_state_packet);
  putMqttSendDelay(mqtt_send_delay);
  putUpTimeSendInterval(upTimeSendInterval);
  #endif

  strcpy(ntpServerName, DEFAULT_NTP_SERVER);
  putNtpServer(String(ntpServerName));

  putAM1hour(AM1_hour);                 // Режим 1 по времени - часы
  putAM1minute(AM1_minute);             // Режим 1 по времени - минуты
  putAM1effect(AM1_effect_id);          // Режим 1 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM2hour(AM2_hour);                 // Режим 2 по времени - часы
  putAM2minute(AM2_minute);             // Режим 2 по времени - минуты
  putAM2effect(AM2_effect_id);          // Режим 2 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM3hour(AM3_hour);                 // Режим 3 по времени - часы
  putAM3minute(AM3_minute);             // Режим 3 по времени - минуты
  putAM3effect(AM3_effect_id);          // Режим 3 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM4hour(AM4_hour);                 // Режим 4 по времени - часы
  putAM4minute(AM4_minute);             // Режим 4 по времени - минуты
  putAM4effect(AM4_effect_id);          // Режим 4 по времени - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM5effect(dawn_effect_id);         // Режим по времени "Рассвет" - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
  putAM6effect(dusk_effect_id);         // Режим по времени "Закат"   - действие: -3 - выключено (не используется); -2 - выключить матрицу (черный экран); -1 - огонь, 0 - случайный, 1 и далее - эффект EFFECT_LIST
       
  putStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]);

  putCurrentManualMode(-1);             // Текущий вручную включенный режим
}

void saveSettings() {

  saveSettingsTimer.reset();
  if (!eepromModified) return;
  
  // Поставить отметку, что EEPROM инициализировано параметрами эффектов
  EEPROMwrite(0, EEPROM_OK);
  
  EEPROM.commit();
  Serial.println(F("Настройки сохранены в EEPROM"));
  eepromModified = false;
}


byte getMaxBrightness() {
  return EEPROMread(1);
}

void putMaxBrightness(byte brightness) {
  if (brightness != getMaxBrightness()) {
    EEPROMwrite(1, brightness);
  }
}

void putAutoplay(boolean value) {
  if (value != getAutoplay()) {
    EEPROMwrite(2, value ? 1 : 0);
  }  
}

bool getAutoplay() {
  return EEPROMread(2) == 1;
}

void putAutoplayTime(long value) {
  if (value != getAutoplayTime()) {
    EEPROMwrite(3, constrain(value / 1000L, 0, 255));
  }
}

long getAutoplayTime() {
  long time = EEPROMread(3) * 1000L;  
  if (time == 0) time = ((long)AUTOPLAY_PERIOD * 1000L);
  return time;
}

void putIdleTime(long value) {
  if (value != getIdleTime()) {
    EEPROMwrite(4, constrain(value / 60L / 1000L, 0, 255));
  }
}

uint16_t getUpTimeSendInterval() {
  return EEPROM_int_read(280);
}

void putUpTimeSendInterval(uint16_t value) {
  if (value != getUpTimeSendInterval()) {
    EEPROM_int_write(280, value);
  }
}

long getIdleTime() {
  long time = EEPROMread(4) * 60 * 1000L;  
  return time;
}

void putUseNtp(boolean value) {
  if (value != getUseNtp()) {
    EEPROMwrite(5, value);
  }
}

bool getUseNtp() {
  return EEPROMread(5) == 1;
}

void putNtpSyncTime(uint16_t value) {
  if (value != getNtpSyncTime()) {
    EEPROM_int_write(6, value);
  }
}

uint16_t getNtpSyncTime() {
  uint16_t time = EEPROM_int_read(6);  
  if (time == 0) time = 60;
  return time;
}

void putTimeZone(int8_t value) {
  if (value != getTimeZone()) {
    EEPROMwrite(8, (byte)value);
  }
}

int8_t getTimeZone() {
  return (int8_t)EEPROMread(8);
}

bool getTurnOffClockOnLampOff() {
  return EEPROMread(9) == 1;
}

void putTurnOffClockOnLampOff(bool flag) {
  if (flag != getTurnOffClockOnLampOff()) {
    EEPROMwrite(9, flag ? 1 : 0);
  }  
}

void putAlarmParams(byte alarmWeekDay, byte dawnDuration, byte alarmEffect, byte alarmDuration) {  
  if (alarmWeekDay != getAlarmWeekDay()) {
    EEPROMwrite(20, alarmWeekDay);
  }
  // if (dawnDuration != getDawnDuration()) {
  //   EEPROMwrite(21, dawnDuration);
  // }
  // if (alarmEffect != getAlarmEffect()) {
  //   EEPROMwrite(22, alarmEffect);
  // }
  // //   24 - Будильник, длительность звука будильника, минут
  // if (alarmDuration != getAlarmDuration()) {
  //   EEPROMwrite(24, alarmDuration);
  // }
}

byte getAlarmHour(byte day) { 
  return constrain(EEPROMread(40 + 2 * (day - 1)), 0, 23);
}

byte getAlarmMinute(byte day) { 
  return constrain(EEPROMread(40 + 2 * (day - 1) + 1), 0, 59);
}

void putAlarmTime(byte day, byte hour, byte minute) { 
  if (hour != getAlarmHour(day)) {
    EEPROMwrite(40 + 2 * (day - 1), constrain(hour, 0, 23));
  }
  if (minute != getAlarmMinute(day)) {
    EEPROMwrite(40 + 2 * (day - 1) + 1, constrain(minute, 0, 59));
  }
}

byte getAlarmWeekDay() { 
  return EEPROMread(20);
}

bool getUseSoftAP() {
  return EEPROMread(14) == 1;
}

void putUseSoftAP(boolean use) {  
  if (use != getUseSoftAP()) {
    EEPROMwrite(14, use ? 1 : 0);
  }
}

String getSoftAPName() {
  return EEPROM_string_read(54, 10);
}

void putSoftAPName(String SoftAPName) {
  if (SoftAPName != getSoftAPName()) {
    EEPROM_string_write(54, SoftAPName, 10);
  }
}

String getSoftAPPass() {
  return EEPROM_string_read(64, 16);
}

void putSoftAPPass(String SoftAPPass) {
  if (SoftAPPass != getSoftAPPass()) {
    EEPROM_string_write(64, SoftAPPass, 16);
  }
}

String getSsid() {
  return EEPROM_string_read(80, 24);
}

void putSsid(String Ssid) {
  if (Ssid != getSsid()) {
    EEPROM_string_write(80, Ssid, 24);
  }
}

String getPass() {
  return EEPROM_string_read(104, 16);
}

void putPass(String Pass) {
  if (Pass != getPass()) {
    EEPROM_string_write(104, Pass, 16);
  }
}

String getNtpServer() {
  return EEPROM_string_read(120, 30);
}

void putNtpServer(String server) {
  if (server != getNtpServer()) {
    EEPROM_string_write(120, server, 30);
  }
}

void putAM1params(byte hour, byte minute, int8_t effect) { 
  putAM1hour(hour);
  putAM1minute(minute);
  putAM1effect(effect);
}

byte getAM1hour() { 
  byte hour = EEPROMread(33);
  if (hour>23) hour = 0;
  return hour;
}

void putAM1hour(byte hour) {
  if (hour != getAM1hour()) {
    EEPROMwrite(33, hour);
  }
}

byte getAM1minute() {
  byte minute = EEPROMread(34);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM1minute(byte minute) {
  if (minute != getAM1minute()) {
    EEPROMwrite(34, minute);
  }
}

int8_t getAM1effect() {
  int8_t value = (int8_t)EEPROMread(35);
  if (value < -3) value = -3;
  return value;
}

void putAM1effect(int8_t effect) {
  if (effect != getAM1effect()) {
    EEPROMwrite(35, (byte)effect);
  }
}

void putAM2params(byte hour, byte minute, int8_t effect) { 
  putAM2hour(hour);
  putAM2minute(minute);
  putAM2effect(effect);
}

byte getAM2hour() { 
  byte hour = EEPROMread(36);
  if (hour>23) hour = 0;
  return hour;
}

void putAM2hour(byte hour) {
  if (hour != getAM2hour()) {
    EEPROMwrite(36, hour);
  }
}

byte getAM2minute() {
  byte minute = EEPROMread(37);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM2minute(byte minute) {
  if (minute != getAM2minute()) {
    EEPROMwrite(37, minute);
  }
}

int8_t getAM2effect() {
  int8_t value = (int8_t)EEPROMread(38);
  if (value < -3) value = -3;
  return value;
}

void putAM2effect(int8_t effect) {
  if (effect != getAM2effect()) {
    EEPROMwrite(38, (byte)effect);
  }
}

void putAM3params(byte hour, byte minute, int8_t effect) { 
  putAM3hour(hour);
  putAM3minute(minute);
  putAM3effect(effect);
}

byte getAM3hour() { 
  byte hour = EEPROMread(161);
  if (hour>23) hour = 0;
  return hour;
}

void putAM3hour(byte hour) {
  if (hour != getAM3hour()) {
    EEPROMwrite(161, hour);
  }
}

byte getAM3minute() {
  byte minute = EEPROMread(162);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM3minute(byte minute) {
  if (minute != getAM3minute()) {
    EEPROMwrite(162, minute);
  }
}

int8_t getAM3effect() {
  int8_t value = (int8_t)EEPROMread(163);
  if (value < -3) value = -3;
  return value;
}

void putAM3effect(int8_t effect) {
  if (effect != getAM3effect()) {
    EEPROMwrite(163, (byte)effect);
  }
}

void putAM4params(byte hour, byte minute, int8_t effect) { 
  putAM4hour(hour);
  putAM4minute(minute);
  putAM4effect(effect);
}

byte getAM4hour() { 
  byte hour = EEPROMread(164);
  if (hour>23) hour = 0;
  return hour;
}

void putAM4hour(byte hour) {
  if (hour != getAM4hour()) {
    EEPROMwrite(164, hour);
  }
}

byte getAM4minute() {
  byte minute = EEPROMread(165);
  if (minute > 59) minute = 0;
  return minute;
}

void putAM4minute(byte minute) {
  if (minute != getAM4minute()) {
    EEPROMwrite(165, minute);
  }
}

int8_t getAM4effect() {
  int8_t value = (int8_t)EEPROMread(166);
  if (value < -3) value = -3;
  return value;
}

void putAM4effect(int8_t effect) {
  if (effect != getAM4effect()) {
    EEPROMwrite(166, (byte)effect);
  }
}

int8_t getAM5effect() {
  int8_t value = (int8_t)EEPROMread(243);
  if (value < -3) value = -3;
  return value;
}

void putAM5effect(int8_t effect) {
  if (effect != getAM5effect()) {
    EEPROMwrite(243, (byte)effect);
  }
}

int8_t getAM6effect() {
  int8_t value = (int8_t)EEPROMread(248);
  if (value < -3) value = -3;
  return value;
}

void putAM6effect(int8_t effect) {
  if (effect != getAM6effect()) {
    EEPROMwrite(248, (byte)effect);
  }
}

int8_t getCurrentManualMode() {
  return (int8_t)EEPROMread(29);
}

void putCurrentManualMode(int8_t mode) {
  if (mode != getCurrentManualMode()) {
    EEPROMwrite(29, (byte)mode);
  }
}

void getStaticIP() {
  IP_STA[0] = EEPROMread(10);
  IP_STA[1] = EEPROMread(11);
  IP_STA[2] = EEPROMread(12);
  IP_STA[3] = EEPROMread(13);
}

void putStaticIP(byte p1, byte p2, byte p3, byte p4) {
  EEPROMwrite(10, p1);
  EEPROMwrite(11, p2);
  EEPROMwrite(12, p3);
  EEPROMwrite(13, p4);
}

#if (USE_MQTT == 1)

bool getUseMqtt() {
  return EEPROMread(239) == 1;
}

void putUseMqtt(boolean use) {  
  if (use != getUseMqtt()) {
    EEPROMwrite(239, use ? 1 : 0);
  }
}

bool getSendStateInPacket() {
  return EEPROMread(249) == 1;
}

void putSendStateInPacket(boolean use_packet) {  
  if (use_packet != getSendStateInPacket()) {
    EEPROMwrite(249, use_packet ? 1 : 0);
  }
}

uint16_t getMqttPort() {
  uint16_t val = (uint16_t)EEPROM_int_read(237);
  return val;
}

void putMqttPort(uint16_t port) {
  if (port != getMqttPort()) {
    EEPROM_int_write(237, port);
  }  
}

String getMqttServer() {
  return EEPROM_string_read(182, 24);
}

void putMqttServer(String server) {
  if (server != getMqttServer()) {
    EEPROM_string_write(182, server, 24);
  }
}

String getMqttUser() {
  return EEPROM_string_read(207, 24);
}

void putMqttUser(String user) {
  if (user != getMqttUser()) {
    EEPROM_string_write(207, user, 14);
  }
}

String getMqttPass() {
  return EEPROM_string_read(222, 24);
}

void putMqttPass(String pass) {
  if (pass != getMqttPass()) {
    EEPROM_string_write(222, pass, 14);
  }
}

String getMqttPrefix() {
  return EEPROM_string_read(250, 30);
}

void putMqttPrefix(String prefix) {  
  if (prefix != getMqttPrefix()) {
    EEPROM_string_write(250, prefix, 30);
  }
}

uint16_t getMqttSendDelay() {
  uint16_t val = (uint16_t)EEPROM_int_read(241);
  return val;
}

void putMqttSendDelay(uint16_t port) {
  if (port != getMqttSendDelay()) {
    EEPROM_int_write(241, port);
  }  
}

#endif

// ----------------------------------------------------------

byte EEPROMread(uint16_t addr) {    
  return EEPROM.read(addr);
}

void EEPROMwrite(uint16_t addr, byte value) {    
  EEPROM.write(addr, value);
  eepromModified = true;
  saveSettingsTimer.reset();
}

// чтение uint16_t
uint16_t EEPROM_int_read(uint16_t addr) {    
  byte raw[2];
  for (byte i = 0; i < 2; i++) raw[i] = EEPROMread(addr+i);
  uint16_t &num = (uint16_t&)raw;
  return num;
}

// запись uint16_t
void EEPROM_int_write(uint16_t addr, uint16_t num) {
  byte raw[2];
  (uint16_t&)raw = num;
  for (byte i = 0; i < 2; i++) EEPROMwrite(addr+i, raw[i]);
}

// чтение uint32_t
uint32_t EEPROM_long_read(uint16_t addr) {    
  byte raw[4];
  for (byte i = 0; i < 4; i++) raw[i] = EEPROMread(addr+i);
  uint32_t &num = (uint32_t&)raw;
  return num;
}

// запись uint32_t
void EEPROM_long_write(uint16_t addr, uint32_t num) {
  byte raw[4];
  (uint32_t&)raw = num;
  for (byte i = 0; i < 4; i++) EEPROMwrite(addr+i, raw[i]);
}

String EEPROM_string_read(uint16_t addr, int16_t len) {
   char buffer[len+1];
   memset(buffer,'\0',len+1);
   int16_t i = 0;
   while (i < len) {
     byte c = EEPROMread(addr+i);
     if (c == 0) break;
     buffer[i++] = c;
   }
   return String(buffer);
}

void EEPROM_string_write(uint16_t addr, String buffer, uint16_t max_len) {
  uint16_t len = buffer.length();
  uint16_t i = 0;

  // Принудительно очистить "хвосты от прежнего значения"
  while (i < max_len) {
    EEPROMwrite(addr+i, 0x00);
    i++;
  }
  
  // Обрезать строку, если ее длина больше доступного места
  if (len > max_len) len = max_len;

  // Сохранить новое значение
  i = 0;
  while (i < len) {
    EEPROMwrite(addr+i, buffer[i]);
    i++;
  }
}

// Проверка наличия сохраненной резервной копии
// Возврат: 0 - не найден; 1 - найден в FS микроконтроллера; 2 - найден на SD-карте; 3 - найден в FS и на SD
uint8_t checkEepromBackup() {
  File file;
  String  fileName = F("/eeprom.bin");
  uint8_t existsFS = 0; 
  size_t  fs_size;
  
  file = LittleFS.open(fileName, "r");
  if (file) {
    fs_size = file.size();
    if (file.size() == EEPROM_MAX) {
      existsFS = 1;
    }
    file.close();
  }
  
  return existsFS;
}

// Сохранить eeprom в файл
// storage = "FS" - внутренняя файловая система
// storage = "SD" - на SD-карту
// возврат: true - успех; false - ошибка
bool saveEepromToFile(String storage) {

  const uint8_t part_size = 128;
  bool ok = true;
  uint8_t buf[part_size];
  String message = "", fileName = F("/eeprom.bin");
  size_t len = 0;
  uint16_t cnt = 0, idx = 0;  
  File file;

  saveSettings();
  storage = "FS";

  Serial.print(F("Сохранение файла: "));
  Serial.println(storage + String(F(":/")) + fileName);

  memset(buf, 0, part_size);

  if (storage == "FS") {

    // Если файл с таким именем уже есть - удалить (перезапись файла новым)
    if (LittleFS.exists(fileName)) {
      ok = LittleFS.remove(fileName);
      if (!ok) {
        message = String(F("Ошибка создания файла '")) + fileName + "'";
        Serial.println(message);
        return false;
      }
    }
  
    file = LittleFS.open(fileName, "w");
  }

  if (!file) {
    message = String(F("Ошибка создания файла '")) + fileName + "'";
    Serial.println(message);
    return false;
  }

  while (idx < EEPROM_MAX) {
    delay(0);
    if (cnt >= part_size) {
      len = file.write(buf, cnt);
      ok = len == cnt;
      if (!ok) break;
      cnt = 0;
      memset(buf, 0, part_size);
    }
    buf[cnt++] = EEPROMread(idx++);
  }

  // Дописываем остаток
  if (ok && cnt > 0) {
    len = file.write(buf, cnt);
    ok = len == cnt;
  }
  
  if (!ok) {
    message = String(F("Ошибка записи в файл '")) + fileName + "'";
    Serial.println(message);
    file.close();
    return false;
  }          
  
  file.close();
  Serial.println(F("Файл сохранен."));

  eeprom_backup = checkEepromBackup();
  
  return true;
}

// Загрузить eeprom из файла
// storage = "FS" - внутренняя файловая система
// storage = "SD" - на SD-карту
// возврат: true - успех; false - ошибка
bool loadEepromFromFile(String storage) {

  const uint8_t part_size = 128;
  bool ok = true;
  uint8_t buf[part_size];
  String message = "", fileName = F("/eeprom.bin");
  size_t len = 0;
  uint16_t idx = 0;  
  File file;

  storage = "FS";

  Serial.print(F("Загрузка файла: "));
  Serial.println(storage + String(F(":/")) + fileName);

  if (storage == "FS") {
    file = LittleFS.open(fileName, "r");
  }

  if (!file) {
    message = String(F("Файл '")) + fileName + String(F("' не найден."));
    Serial.println(message);
    return false;
  }
  
  clearEEPROM();
  
  while (idx < EEPROM_MAX) {
    delay(0);
    memset(buf, 0, part_size);
    len = file.read(buf, part_size);
    for (uint8_t i=0; i<len; i++) {
      EEPROMwrite(idx++, buf[i]);
    }
  }
  file.close();

  ok = idx == EEPROM_MAX;

  if (!ok) {
    message = String(F("Ошибка чтения файла '")) + fileName + "'";
    Serial.println(message);
    return false;
  }          

  // Записать в 0 текущее значение EEPROM_OK, иначе при несовпадении версии
  // после перезагрузки будут восстановлены значения по-умолчанию
  EEPROMwrite(0, EEPROM_OK);
  
  saveSettings();
  
  return true;
}

// ----------------------------------------------------------
