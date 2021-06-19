#ifndef EEPROM1_H
#define EEPROM1_H
void loadSettings();
void clearEEPROM();
void saveDefaults();
void saveSettings();
uint16_t getUpTimeSendInterval();
void putUpTimeSendInterval(uint16_t value);

float getMaxHum();
void putMaxHum(float value);
float getMinHum();
void putMinHum(float value);

void putUseNtp(boolean value);
bool getUseNtp();
void putNtpSyncTime(uint16_t value);
uint16_t getNtpSyncTime();
void putTimeZone(int8_t value);
int8_t getTimeZone();
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
void getStaticIP();
void putStaticIP(byte p1, byte p2, byte p3, byte p4);

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
void EEPROMWriteFloat(uint16_t addr, float val); // запись в ЕЕПРОМ

float EEPROMReadFloat(uint16_t addr); // чтение из ЕЕПРОМ

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