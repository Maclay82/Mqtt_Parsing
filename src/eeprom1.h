#ifndef EEPROM1_H
#define EEPROM1_H
void loadSettings();
void clearEEPROM();
void saveDefaults();
void saveSettings();
// void putEffectParams(byte effect, int speed, boolean use, boolean use_text_overlay, boolean use_clock_overlay, byte value1, byte value2, byte contrast);
// void putEffectSpeed(byte effect, int speed);
// byte getEffectSpeed(byte effect);
// boolean getEffectUsage(byte effect);
// void putEffectUsage(byte effect, boolean use);
// boolean getEffectTextOverlayUsage(byte effect);
// void putEffectTextOverlayUsage(byte effect, boolean use);
// boolean getEffectClockOverlayUsage(byte effect);
// void putEffectClockOverlayUsage(byte effect, boolean use);
// void putScaleForEffect(byte effect, byte value){}
// byte getScaleForEffect(byte effect){return effect;}
// void putScaleForEffect2(byte effect, byte value){}
// byte getScaleForEffect2(byte effect){return effect;}
byte getMaxBrightness();
void putMaxBrightness(byte brightness);
void putAutoplay(boolean value);
bool getAutoplay();
void putAutoplayTime(long value);
long getAutoplayTime();
void putIdleTime(long value);
uint16_t getUpTimeSendInterval();
void putUpTimeSendInterval(uint16_t value);
long getIdleTime();
void putUseNtp(boolean value);
bool getUseNtp();
void putNtpSyncTime(uint16_t value);
uint16_t getNtpSyncTime();
void putTimeZone(int8_t value);
int8_t getTimeZone();
bool getTurnOffClockOnLampOff();
void putTurnOffClockOnLampOff(bool flag);
void putAlarmParams(byte alarmWeekDay, byte dawnDuration, byte alarmEffect, byte alarmDuration);
byte getAlarmHour(byte day);
byte getAlarmMinute(byte day);
void putAlarmTime(byte day, byte hour, byte minute);
byte getAlarmWeekDay();
bool getUseSoftAP();
void putUseSoftAP(boolean use);
String getSoftAPName();
void putSoftAPName(String SoftAPName);
String getSoftAPPass();
void putSoftAPPass(String SoftAPPass);
String getSsid();
void putSsid(String Ssid);
String getPass();
void putPass(String Pass);
String getNtpServer();
void putNtpServer(String server);
void putAM1params(byte hour, byte minute, int8_t effect);
byte getAM1hour();
void putAM1hour(byte hour);
byte getAM1minute();
void putAM1minute(byte minute);
int8_t getAM1effect();
void putAM1effect(int8_t effect);
void putAM2params(byte hour, byte minute, int8_t effect);
byte getAM2hour();
void putAM2hour(byte hour);
byte getAM2minute();
void putAM2minute(byte minute);
int8_t getAM2effect();
void putAM2effect(int8_t effect);
void putAM3params(byte hour, byte minute, int8_t effect);
byte getAM3hour();
void putAM3hour(byte hour);
byte getAM3minute();
void putAM3minute(byte minute);
int8_t getAM3effect();
void putAM3effect(int8_t effect);
void putAM4params(byte hour, byte minute, int8_t effect);
byte getAM4hour();
void putAM4hour(byte hour);
byte getAM4minute();
void putAM4minute(byte minute);
int8_t getAM4effect();
void putAM4effect(int8_t effect);
int8_t getAM5effect();
void putAM5effect(int8_t effect);
int8_t getAM6effect();
void putAM6effect(int8_t effect);
int8_t getCurrentManualMode();
void putCurrentManualMode(int8_t mode);
void getStaticIP();
void putStaticIP(byte p1, byte p2, byte p3, byte p4);
// Загрузка массива строк "Бегущей строки"
void loadTexts();

#if (USE_MQTT == 1)

bool getUseMqtt();
void putUseMqtt(boolean use);
bool getSendStateInPacket();
void putSendStateInPacket(boolean use_packet);
uint16_t getMqttPort();
void putMqttPort(uint16_t port);
String getMqttServer();
void putMqttServer(String server);
String getMqttUser();
void putMqttUser(String user);
String getMqttPass();
void putMqttPass(String pass);
String getMqttPrefix();
void putMqttPrefix(String prefix);
uint16_t getMqttSendDelay();
void putMqttSendDelay(uint16_t port);
#endif

// ----------------------------------------------------------

byte EEPROMread(uint16_t addr);

void EEPROMwrite(uint16_t addr, byte value);
// чтение uint16_t
uint16_t EEPROM_int_read(uint16_t addr);
// запись uint16_t
void EEPROM_int_write(uint16_t addr, uint16_t num);
// чтение uint32_t
uint32_t EEPROM_long_read(uint16_t addr);
// запись uint32_t
void EEPROM_long_write(uint16_t addr, uint32_t num);

String EEPROM_string_read(uint16_t addr, int16_t len);
void EEPROM_string_write(uint16_t addr, String buffer, uint16_t max_len);

// Проверка наличия сохраненной резервной копии
// Возврат: 0 - не найден; 1 - найден в FS микроконтроллера; 2 - найден на SD-карте; 3 - найден в FS и на SD
uint8_t checkEepromBackup();

// Сохранить eeprom в файл
// storage = "FS" - внутренняя файловая система
// возврат: true - успех; false - ошибка
bool saveEepromToFile(String storage);

// Загрузить eeprom из файла
// storage = "FS" - внутренняя файловая система
// возврат: true - успех; false - ошибка
bool loadEepromFromFile(String storage);

// ----------------------------------------------------------
#endif