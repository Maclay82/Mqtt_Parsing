#ifndef EEPROM1_H
#define EEPROM1_H
void loadSettings();
void clearEEPROM();
void saveDefaults();
void saveSettings();
uint16_t getUpTimeSendInterval();
void putUpTimeSendInterval(uint16_t value);

#ifdef HUMCONTROL  
float getMaxHum();
void putMaxHum(float value);
float getMinHum();
void putMinHum(float value);
#endif

#ifdef PHTDSCONTROL
void putRAWMode (boolean value);  //putRAWMode режим чтения "сырых" данных с Ph TDS
bool getRAWMode();                //getRAWMode режим чтения "сырых" данных с Ph TDS
void putPhKa (uint16_t value);    // усиление
uint16_t getPhKa ();              // усиление
void putPhKb (uint16_t value);    // ст
uint16_t getPhKb ();              // ст
void putTDSKa (uint16_t value);   // усиление
uint16_t getTDSKa ();             // усиление
void putTDSKb (uint16_t value);   //средняя точка
uint16_t getTDSKb ();             //средняя точка
void putRawPhCalP1 (uint16_t value);    //Загрузка 1ой калибровочной точки Ph (сырые данные)  //  24 - rawPhCalP1
uint16_t getRawPhCalP1 ();              //Выгрузка 1ой калибровочной точки (сырые данные)
void putRawPhCalP2 (uint16_t value);    //Загрузка 2ой калибровочной точки Ph (сырые данные)  //  26 - rawPhCalP2
uint16_t getRawPhCalP2 ();              //Выгрузка 2ой калибровочной точки (сырые данные)
void putRawTDSCalP1 (uint16_t value);   //Загрузка 1ой калибровочной точки TDS (сырые данные)  //  28 - rawTDSCalP1
uint16_t getRawTDSCalP1 ();             //Выгрузка 1ой калибровочной точки (сырые данные)
void putRawTDSCalP2 (uint16_t value);   //Загрузка 2ой калибровочной точки TDS (сырые данные)  //  30 - rawTDSCalP2
uint16_t getRawTDSCalP2 ();             //Выгрузка 2ой калибровочной точки (сырые данные)
void putPhCalP1 (float value);          //Загрузка 1ой калибровочной точки Ph            //  32 - PhCalP1
float getPhCalP1();                     //Выгрузка 1ой калибровочной точки Ph 
void putPhCalP2 (float value);          //Загрузка 2ой калибровочной точки Ph             //  36 - PhCalP2
float getPhCalP2();                     //Выгрузка 2ой калибровочной точки Ph 
void putTDSCalP1 (uint16_t value);      //Загрузка 1ой калибровочной точки TDS       //  40 - TDSCalP1
uint16_t getTDSCalP1 ();                //Выгрузка 1ой калибровочной точки
void putTDSCalP2 (uint16_t value);      //Загрузка 2ой калибровочной точки TDS       //  42 - TDSCalP2
uint16_t getTDSCalP2 ();                //Выгрузка 2ой калибровочной точки

void putPhmin (float value);  // 66 - phmin
float getPhmin();
void putPhmax (float value);  // 70 - phmax
float getPhmax();
void putTDSmin (uint16_t value); // 74 - tdsmin
uint16_t getTDSmin ();
void putTDSmax (uint16_t value);  // 76 - tdsmax
uint16_t getTDSmax ();

float getPumpScl(int numpump);
void putPumpScl(float value, int numpump);
void putPumpCalVol (uint16_t value, int numpump);
uint16_t getPumpCalVol(int numpump);
#endif


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

uint16_t EEPROM_int_read(uint16_t addr);            // чтение uint16_t
void EEPROM_int_write(uint16_t addr, uint16_t num); // запись uint16_t
uint32_t EEPROM_long_read(uint16_t addr);           // чтение uint32_t
void EEPROM_long_write(uint16_t addr, uint32_t num);// запись uint32_t

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