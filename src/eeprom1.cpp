#include <Arduino.h>
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

bool     eepromModified = false;            // флаг: EEPROM изменен, требует сохранения
// ------------------- ФАЙЛОВАЯ СИСТЕМА SPIFFS ----------------------

bool       spiffs_ok = false;                    // Флаг - файловая система SPIFFS доступна для использования
size_t     spiffs_total_bytes;                   // Доступно байт в SPIFFS
size_t     spiffs_used_bytes;                    // Использовано байт в SPIFFS
int8_t     eeprom_backup = 0;                    // Флаг - backup настроек 0 - нeт; 1 - FS; 2 - SD; 3 - FS и SD


void loadSettings() {

  // Адреса в EEPROM:
  //   0 - если EEPROM_OK - EEPROM инициализировано, если другое значение - нет                             // EEPROMread(0)                 // EEPROMWrite(0, EEPROM_OK)

  //   4 - Текущий режим работы (0-бак подготовки воды, 1-бак расствора)
  //   5 - использовать синхронизацию времени через NTP                                                     // getUseNtp()                   // putUseNtp(useNtp)
  // 6,7 - период синхронизации NTP (int16_t - 2 байта) в минутах                                           // getNtpSyncTime()              // putNtpSyncTime(syncTimePeriod)
  //   8 - time zone UTC+X                                                                                  // getTimeZone();                // putTimeZone(timeZoneOffset)
  //   9 - Получать Динамический IP                                                                         // getUseDHCP()                  // putUseDHCP()
  //  10 - IP[0]                                                                                            // getStaticIP()                 // putStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3])
  //  11 - IP[1]                                                                                            // - " -                         // - " -
  //  12 - IP[2]                                                                                            // - " -                         // - " -
  //  13 - IP[3]                                                                                            // - " -                         // - " -
  //  14 - Использовать режим точки доступа                                                                 // getUseSoftAP()                // putUseSoftAP(useSoftAP) 
  //  15 - RAWMode              //getRAWMode, putRAWMode режим чтения "сырых" данных с Ph TDS
  //  16 - PhKa                 //getPhKa, putPhKa усиление аппаратное
  //  18 - PhKb                 //getPhKb, putPhKb средняя точка аппаратная
  //  20 - TDSKa                //getTDSKa, putTDSKa усиление аппаратное
  //  22 - TDSKb                //getTDSKb, putTDSKb средняя точка аппаратная
  //  24 - rawPhCalP1
  //  26 - rawPhCalP2
  //  28 - rawTDSCalP1
  //  30 - rawTDSCalP2
  //  32 - PhCalP1
  //  36 - PhCalP2
  //  40 - TDSCalP1
  //  42 - TDSCalP2
  //  44 - tdsAVol
  //  46 - tdsBVol
  //  48 - tdsCVol
  //  50 - 
  //  52 -



  // 62 - PhVol
  // 64 - regDelay
  // 66 - phmin
  // 70 - phmax
  // 74 - tdsmin
  // 76 - tdsmax
  // 78 - maxhum                                                                                             // getMaxHum()                   // putMaxHum(maxhum)
  // 82 - minhum                                                                                             // getMinHum()                   // putMinHum(minhum)  
  // 86-95   - имя точки доступа    - 10 байт                                                                // getSoftAPName().toCharArray(apName, 10)       // putSoftAPName(String(apName))       // char apName[11] = ""
  // 96-111  - пароль точки доступа - 16 байт                                                                // getSoftAPPass().toCharArray(apPass, 17)       // putSoftAPPass(String(apPass))       // char apPass[17] = "" 
  // 112-135 - имя сети  WiFi       - 24 байта                                                               // getSsid().toCharArray(ssid, 25)               // putSsid(String(ssid))               // char ssid[25]   = ""
  // 136-151 - пароль сети  WiFi    - 16 байт                                                                // getPass().toCharArray(pass, 17)               // putPass(String(pass))               // char pass[17]   = ""
  // 152-181 - имя NTP сервера      - 30 байт                                                                // getNtpServer().toCharArray(ntpServerName, 31) // putNtpServer(String(ntpServerName)) // char ntpServerName[31] = ""
  // 182-206 - MQTT сервер (24 симв)                                                                         // getMqttServer().toCharArray(mqtt_server, 24)  // putMqttServer(String(mqtt_server))       // char mqtt_server[25] = ""
  // 207-221 - MQTT user (14 симв)                                                                           // getMqttUser().toCharArray(mqtt_user, 14)      // putMqttUser(String(mqtt_user))           // char mqtt_user[15] = ""
  // 222-236 - MQTT pwd (14 симв)                                                                            // getMqttPass().toCharArray(mqtt_pass, 14)      // putMqttPass(String(mqtt_pass))           // char mqtt_pass[15] = ""
  // 237,238 - MQTT порт                                                                                     // getMqttPort()                  // putMqttPort(mqtt_port)
  // 239 - использовать MQTT канал управления: 0 - нет 1 - да                                                // getUseMqtt()                   // putUseMqtt(useMQTT)  
  // 240 - отправка параметров состояния в MQTT 0 - индивидуально, 1 - пакетами                              // getSendStateInPacket()         // putSendStateInPacket(mqtt_state_packet)  
  // 241,242 - задержка отправкии запросов MQTT серверу                                                      // getMqttSendDelay()             // putMqttSendDelay(mqtt_send_delay)
  // 243-272 - префикс топика сообщения (30 симв)                                                            // getMqttPrefix()                // putMqttPrefix(mqtt_prefix)
  // 273,274 - интервал отправки значения uptime на MQTT-сервер                                              // getUpTimeSendInterval()        // putUpTimeSendInterval(upTimeSendInterval)
  
  //**275 - не используется
  //  ...
  //  400-... //PumpScale коэффициенты нвсосов
  //**499 - не используется

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
    useNtp = getUseNtp();
    timeZoneOffset = getTimeZone();

    syncTimePeriod = getNtpSyncTime();
    useSoftAP = getUseSoftAP();
    getSoftAPName().toCharArray(apName, 10);        //  54-63   - имя точки доступа    ( 9 байт макс) + 1 байт '\0'
    getSoftAPPass().toCharArray(apPass, 17);        //  64-79   - пароль точки доступа (16 байт макс) + 1 байт '\0'
    getSsid().toCharArray(ssid, 25);                //  80-103  - имя сети  WiFi       (24 байта макс) + 1 байт '\0'
    getPass().toCharArray(pass, 17);                //  104-119 - пароль сети  WiFi    (16 байт макс) + 1 байт '\0'
    getNtpServer().toCharArray(ntpServerName, 31);  //  120-149 - имя NTP сервера      (30 байт макс) + 1 байт '\0'
    useDHCP = getUseDHCP();
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

#ifdef HUMCONTROL  
    maxhum = getMaxHum();
    minhum = getMinHum();
#endif

#ifdef PHTDSCONTROL
    RAWMode = getRAWMode(); // режим чтения "сырых" данных с Ph TDS
    thisMode = getCurrentMode();
    phKa  = getPhKa(); //getPhKa усиление
    phKb  = getPhKb(); //getPhKb средняя точка
    tdsKa = getTDSKa(); //getTDSKa усиление
    tdsKb = getTDSKb(); //getTDSKb средняя точка
    phVol = getPhVol();
    tdsAVol = getTdsAVol();
    tdsBVol = getTdsBVol();
    tdsCVol = getTdsCVol();
    regDelay = getregDelay() * 1000 * 60;
    phmin = getPhmin();
    phmax = getPhmax();
    tdsmin = getTDSmin();
    tdsmax = getTDSmax();
    PhCalP1 = getPhCalP1(), 
    PhCalP2 = getPhCalP2();
    TDSCalP1 = getTDSCalP1(); 
    TDSCalP2 = getTDSCalP2();
    rawPhCalP1 = getRawPhCalP1();
    rawPhCalP2 = getRawPhCalP2();
    rawTDSCalP1 = getRawTDSCalP1();
    rawTDSCalP2 = getRawTDSCalP2();

    for(int i = 0; i < PUMPCOUNT; i++ ){
//  Serial.print(F("pumps.putPumpScale("));
      pumps.putPumpScale(getPumpScl(i+1),uint8_t(i));
//  Serial.println(F(")"));
    }
#endif

  } 
  else {
    Serial.println(F("Инициализация EEPROM..."));
    // Значения переменных по умолчанию определяются в месте их объявления - в файле def_soft.h
    // Здесь выполняются только инициализация массивов и некоторых специальных параметров
    clearEEPROM();
    // После первой инициализации значений - сохранить их принудительно
    saveDefaults();
    saveSettings();
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
#ifdef HUMCONTROL  
  putMaxHum(maxhumDEF);
  putMinHum(minhumDEF);
#endif

#ifdef PHTDSCONTROL
  putRAWMode      (false);
  putCurrentMode  (1);
  putPhKa         (150);  // усиление
  putPhKb         (125);  // ст
  putTDSKa        (60);  // усиление
  putTDSKb        (110); //средняя точка
  putPhVol        (1);
  putTdsAVol      (1);
  putTdsBVol      (1);
  putTdsCVol      (0);
  putregDelay     (5);

  putPhmin        (6.0); //нижняя граница Ph
  putPhmax        (6.8); //верхняя граница Ph
  putTDSmin       (700); //нижняя граница TDS
  putTDSmax       (1200); //верхняя граница TDS
  putPhCalP1      (4.0); 
  putRawPhCalP1   (802); 
  putPhCalP2      (7.0);
  putRawPhCalP2   (1750);
  putTDSCalP1     (206); 
  putRawTDSCalP1  (220); 
  putTDSCalP2     (1930);
  putRawTDSCalP2  (1924);
  for(int i = 0; i < PUMPCOUNT; i++ ) {
    if(getPumpScl (i+1) <= 0) {
      pumps.putPumpScale (512.82, uint8_t(i));
      putPumpScl(512.82, uint8_t(i+1));
    }
  }
#endif

  putUseNtp(true);
  putTimeZone(TIMEZONE);
  putNtpSyncTime(SYNCTIMEPERIOD);

  putUseSoftAP(false);

  strcpy(apName, DEFAULT_AP_NAME);
  strcpy(apPass, DEFAULT_AP_PASS);
  strcpy(ssid,   NETWORK_SSID);
  strcpy(pass,   NETWORK_PASS);
  useDHCP = USEDHCP;
  #if (USE_MQTT == 1)
  strcpy(mqtt_server, DEFAULT_MQTT_SERVER);
  strcpy(mqtt_user,   DEFAULT_MQTT_USER);
  strcpy(mqtt_pass,   DEFAULT_MQTT_PASS);
  #endif  

  putSoftAPName(String(DEFAULT_AP_NAME));
  putSoftAPPass(String(DEFAULT_AP_PASS));
  putSsid(String(NETWORK_SSID));
  putPass(String(NETWORK_PASS));
  putUseDHCP(USEDHCP);
  #if (USE_MQTT == 1)
  putMqttServer(String(DEFAULT_MQTT_SERVER));
  putMqttUser(String(DEFAULT_MQTT_USER));
  putMqttPass(String(DEFAULT_MQTT_PASS));
  putMqttPrefix(String(DEFAULT_MQTT_PREFIX));
  putMqttPort(DEFAULT_MQTT_PORT);
  putUseMqtt(true);
  putSendStateInPacket(true);
  putMqttSendDelay(MQTT_SEND_DELAY);
  putUpTimeSendInterval(upTimeSendInterval);
  #endif

  strcpy(ntpServerName, DEFAULT_NTP_SERVER);
  putNtpServer(String(ntpServerName));


  putStaticIP(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]);

  EEPROM.commit();
}

void saveSettings() {

  saveSettingsTimer.reset();
  if (!eepromModified) return;
  
  // Поставить отметку, что EEPROM инициализировано
  EEPROMwrite(0, EEPROM_OK);
  
  EEPROM.commit();
  Serial.println(F("Настройки сохранены в EEPROM"));
  eepromModified = false;
}

uint16_t getUpTimeSendInterval() {
  return EEPROM_int_read(273);
}

void putUpTimeSendInterval(uint16_t value) {
  if (value != getUpTimeSendInterval()) {
    EEPROM_int_write(273, value);
  }
}

#ifdef HUMCONTROL  

float getMaxHum() {
  return EEPROMReadFloat(78);
}

void putMaxHum(float value) {
  if (value != getMaxHum() ) {
    EEPROMWriteFloat(78, value);
    EEPROM.commit();
  }
}

float getMinHum() {
  return EEPROMReadFloat(82);
}

void putMinHum(float value) {
  if (value != getMinHum() ) {
    EEPROMWriteFloat(82, value);
    EEPROM.commit();
  }
}
#endif

#ifdef PHTDSCONTROL

int8_t getCurrentMode() {
  return (int8_t)EEPROMread(4);
}

void putCurrentMode(int8_t mode) {
  if (mode != getCurrentMode()) {
    EEPROMwrite(4, (byte)mode);
  }
}

void putRAWMode (boolean value){
  if (value != getRAWMode()) EEPROMwrite(15, value);
}
bool getRAWMode() {
  return EEPROMread(15) == 1;
}
void putPhKa (uint16_t value){  // коэфициент усиления Ph
  if (value != getPhKa ()) EEPROM_int_write(16, value);
}
uint16_t getPhKa (){  // коэфициент усиления Ph
  return EEPROM_int_read(16);
}
void putPhKb (uint16_t value){  // средняя точка Ph
  if (value != getPhKb ()) EEPROM_int_write(18, value);
}
uint16_t getPhKb (){  // средняя точка Ph
  return EEPROM_int_read(18);
}
void putTDSKa (uint16_t value){ // коэфициент усиления TDS
  if (value != getTDSKa ()) EEPROM_int_write(20, value);
} 
uint16_t getTDSKa (){ // коэфициент усиления TDS
  return EEPROM_int_read(20);
}
void putTDSKb (uint16_t value){ //средняя точка TDS
  if (value != getTDSKb ()) EEPROM_int_write(22, value);
}
uint16_t getTDSKb (){ //средняя точка TDS
  return EEPROM_int_read(22);
}

uint16_t getTdsAVol() {
  return EEPROM_int_read(44);
}

void putTdsAVol (uint16_t value)         //  44 - tdsAVol
{
  if (value != getTdsAVol() && value >= 0 ) {
    EEPROM_int_write(44, value);
    EEPROM.commit();
  }
}

uint16_t getTdsBVol() {
  return EEPROM_int_read(46);
}

void putTdsBVol (uint16_t value)           //  46 - tdsBVol
{
  if (value != getTdsBVol() && value >= 0 ) {
    EEPROM_int_write(46, value);
    EEPROM.commit();
  }

}

uint16_t getTdsCVol() {
  return EEPROM_int_read(48);
}

void putTdsCVol (uint16_t value)         //  48 - tdsCVol
{
  if (value != getTdsCVol() && value >= 0 ) {
    EEPROM_int_write(48, value);
    EEPROM.commit();
  }
}

uint16_t getPhVol() {
  return EEPROM_int_read(62);
}

void putPhVol (uint16_t value)         // 62 - phVol
{
  if (value != getPhVol() && value >= 0 ) {
    EEPROM_int_write(62, value);
    EEPROM.commit();
  }

}

uint16_t getregDelay (){
  return EEPROM_int_read(64);
}

void putregDelay (uint16_t value){  // 64 - regDelay
  if (value != getregDelay () && value > 0 ) EEPROM_int_write(64, value);
}

float getPhmin() {
  return EEPROMReadFloat(66);
}

void putPhmin (float value) {  // 66 - phmin
  if (value != getPhmin()) {
    EEPROMWriteFloat(66, value);
    EEPROM.commit();
  }
}

float getPhmax() {
  return EEPROMReadFloat(70);
}

void putPhmax (float value)  // 70 - phmax
{
  if (value != getPhmax() ) {
    EEPROMWriteFloat(70, value);
    EEPROM.commit();
  }
}

void putTDSmin (uint16_t value){ // 74 - tdsmin
  if (value != getTDSmin ()) EEPROM_int_write(74, value);
}
uint16_t getTDSmin (){
  return EEPROM_int_read(74);
}
void putTDSmax (uint16_t value){  // 76 - tdsmax
  if (value != getTDSmax ()) EEPROM_int_write(76, value);
}
uint16_t getTDSmax (){
  return EEPROM_int_read(76);
}
void putRawPhCalP1 (uint16_t value){ //Загрузка 1ой калибровочной точки Ph (сырые данные)  //  24 - rawPhCalP1
  if (value != getTDSmax ()) EEPROM_int_write(24, value);
}
uint16_t getRawPhCalP1 (){ //Выгрузка 1ой калибровочной точки (сырые данные)
  return EEPROM_int_read(24);
}
void putRawPhCalP2 (uint16_t value){ //Загрузка 2ой калибровочной точки Ph (сырые данные)  //  26 - rawPhCalP2
  if (value != getTDSmax ()) EEPROM_int_write(26, value);
}
uint16_t getRawPhCalP2 (){ //Выгрузка 2ой калибровочной точки (сырые данные)
  return EEPROM_int_read(26);
}
void putRawTDSCalP1 (uint16_t value){ //Загрузка 1ой калибровочной точки TDS (сырые данные)  //  28 - rawTDSCalP1
  if (value != getTDSmax ()) EEPROM_int_write(28, value);
}
uint16_t getRawTDSCalP1 (){ //Выгрузка 1ой калибровочной точки (сырые данные)
  return EEPROM_int_read(28);
}
void putRawTDSCalP2 (uint16_t value){ //Загрузка 2ой калибровочной точки TDS (сырые данные)  //  30 - rawTDSCalP2
  if (value != getTDSmax ()) EEPROM_int_write(30, value);
}
uint16_t getRawTDSCalP2 (){ //Выгрузка 2ой калибровочной точки (сырые данные)
  return EEPROM_int_read(30);
}
void putPhCalP1 (float value) { //Загрузка 1ой калибровочной точки Ph //  32 - PhCalP1
  if (value != getPhmin()) {
    EEPROMWriteFloat(32, value);
    EEPROM.commit();
  }
}
float getPhCalP1(){             //Выгрузка 1ой калибровочной точки Ph 
  return EEPROMReadFloat(32);
}
void putPhCalP2 (float value) {//Загрузка 2ой калибровочной точки Ph //  36 - PhCalP2
  if (value != getPhmin()) {
    EEPROMWriteFloat(36, value);
    EEPROM.commit();
  }
}
float getPhCalP2(){           //Выгрузка 1ой калибровочной точки Ph 
  return EEPROMReadFloat(36);
}
void putTDSCalP1 (uint16_t value){ //Загрузка 1ой калибровочной точки TDS   //  40 - TDSCalP1

  if (value != getTDSmax ()) EEPROM_int_write(40, value);
}
uint16_t getTDSCalP1 (){ //Выгрузка 1ой калибровочной точки (сырые данные)
  return EEPROM_int_read(40);
}
void putTDSCalP2 (uint16_t value){ //Загрузка 2ой калибровочной точки TDS   //  42 - TDSCalP2

  if (value != getTDSmax ()) EEPROM_int_write(42, value);
}
uint16_t getTDSCalP2 (){ //Выгрузка 2ой калибровочной точки (сырые данные)
  return EEPROM_int_read(42);
}

float getPumpScl(int numpump){
  // Serial.print("getPumpScl( ");
  // Serial.print(numpump);
  // Serial.print("-");
  // Serial.print("EEPROMReadFloat(");
  // Serial.print(PUMPSCALEADR + ((numpump-1) * 4));
  // Serial.print(")=");
  // Serial.print(EEPROMReadFloat(PUMPSCALEADR + ((numpump-1) * 4)));
  // Serial.println(" ");

  return EEPROMReadFloat(PUMPSCALEADR + ((numpump-1) * 4));
}
void putPumpScl(float value, int numpump){
  // Serial.print("putPumpScl value-");
  // Serial.print(value);
  // Serial.print(" numpump-");
  // Serial.print(numpump);
  // Serial.println(")");
  // Serial.print("+EEPROMWriteFloat(");
  // Serial.print(PUMPSCALEADR + ((numpump-1) * 4));
  // Serial.println(")");

  if (value != getPumpScl(numpump)){
    EEPROMWriteFloat((PUMPSCALEADR + ((numpump-1) * 4)), value);
    EEPROM.commit();
  } 
}

uint16_t getPumpCalVol(int numpump){
  return EEPROM_int_read(PUMPCALVOLADR + ((numpump - 1) * 2));
}
void putPumpCalVol (uint16_t value, int numpump){
  if (value != getTDSKa ()) EEPROM_int_write((PUMPCALVOLADR + ((numpump - 1) * 2)), value);
} 

#endif


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

bool getUseDHCP() {
  return EEPROMread(9) == 1;
}

void putUseDHCP(bool flag) {
  if (flag != getUseDHCP()) {
    EEPROMwrite(9, flag ? 1 : 0);
  }  
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
  return EEPROM_string_read(86, 10);
}

void putSoftAPName(String SoftAPName) {
  if (SoftAPName != getSoftAPName()) {
    EEPROM_string_write(86, SoftAPName, 10);
  }
}

String getSoftAPPass() {
  return EEPROM_string_read(96, 16);
}

void putSoftAPPass(String SoftAPPass) {
  if (SoftAPPass != getSoftAPPass()) {
    EEPROM_string_write(96, SoftAPPass, 16);
  }
}

String getSsid() {
  return EEPROM_string_read(112, 24);
}

void putSsid(String Ssid) {
  if (Ssid != getSsid()) {
    EEPROM_string_write(112, Ssid, 24);
  }
}

String getPass() {
  return EEPROM_string_read(136, 16);
}

void putPass(String Pass) {
  if (Pass != getPass()) {
    EEPROM_string_write(136, Pass, 16);
  }
}

String getNtpServer() {
  return EEPROM_string_read(152, 30);
}

void putNtpServer(String server) {
  if (server != getNtpServer()) {
    EEPROM_string_write(152, server, 30);
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
  return EEPROMread(240) == 1;
}

void putSendStateInPacket(boolean use_packet) {  
  if (use_packet != getSendStateInPacket()) {
    EEPROMwrite(240, use_packet ? 1 : 0);
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
  return EEPROM_string_read(243, 30);
}

void putMqttPrefix(String prefix) {  
  if (prefix != getMqttPrefix()) {
    EEPROM_string_write(243, prefix, 30);
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
  eepromModified = true;
  saveSettingsTimer.reset();
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
  eepromModified = true;
  saveSettingsTimer.reset();
}

void EEPROMWriteFloat(uint16_t addr, float val) // запись в ЕЕПРОМ
{
  byte *x = (byte *)&val;
  for(byte i = 0; i < 4; i++) EEPROM.write(i+addr, x[i]);
  eepromModified = true;
  saveSettingsTimer.reset();
}

float EEPROMReadFloat(uint16_t addr) // чтение из ЕЕПРОМ
{
  byte x[4];
  for(byte i = 0; i < 4; i++) x[i] = EEPROM.read(i+addr);
  float *y = (float *)&x;
  return y[0];
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
  
  file = LittleFS.open(fileName, "r");
  if (file) {
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
