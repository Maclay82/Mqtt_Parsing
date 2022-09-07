#include <Arduino.h>
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

// ----------------------------------------------------
eSources  cmdSource; // Источник команды; NONE - нет значения; BOTH - любой, UDP-клиент, MQTT-клиент
eModes    parseMode; // Текущий режим парсера

unsigned long timing, timing1, timing2, timing3, per, regDelay; // Таймеры опросов, длительность в миллисекундах

#ifdef HUMCONTROL
float temp = 0, humd = 0, humcorr = 3.2,
tempcorr = 0.0;
float minhum, maxhum; // = minCO2PPMEF // = maxCO2PPMEF;
#endif

  
//Инициализация датчика температуры
#ifdef DS18B20
#if defined(ESP8266)
OneWire oneWire(D5); 
#endif
#if defined(ESP32)
OneWire oneWire(15);
#endif
DallasTemperature sensors(&oneWire);
#endif

#ifdef PHTDSCONTROL
extern i2cPumps pumps;

boolean TDScal=false;  //  TDS Calibration start 
boolean PhCal=false;  //  Ph Calibration start
boolean PhOk=false;  //  Ph Correction complete
boolean AutoFill=false;  //  AutoFill is start

boolean RAWMode = true;  // RAW read mode

int rawPh = 0, rawTDS = 0, Wlvl = 0;
int levels[LVLSNSCOUNT];
boolean invLVLsensor[LVLSNSCOUNT] = {true, true, false}; // Инверсия датчиков { hi, mid, low };

float phmin, phmax, phk=1, PhMP=0, tdsk=1, TdsMP=0,
      PhCalP1, PhCalP2; 
float realPh = -1, realTDS = -1, Wtemp = -1;

uint16_t phVol, tdsAVol, tdsBVol, tdsCVol, tdsmin, tdsmax, 
TDSCalP1, TDSCalP2,
rawPhCalP1, rawPhCalP2, 
rawTDSCalP1, rawTDSCalP2,
phKa,  // усиление
phKb,  // средняя точка
tdsKa,  // усиление
tdsKb; // средняя точка
#endif

// ********************* ДЛЯ ПАРСЕРА КОМАНДНЫХ ПАКЕТОВ *************************
 
int32_t    intData[PARSE_AMOUNT];           // массив численных значений после парсинга - для WiFi часы время синхр м.б отрицательным + 
                                            // период синхронизации м.б больше 255 мин - нужен тип int32_t
float      floatData[2];                    // массив дробных численных значений после парсинга
char       incomeBuffer[BUF_MAX_SIZE];      // Буфер для приема строки команды из wifi udp сокета; также используется для загрузки строк из EEPROM
char       replyBuffer[8];                  // ответ клиенту - подтверждения получения команды: "ack;/r/n/0"
byte       ackCounter = 0;                  // счетчик отправляемых ответов для создания уникальности номера ответа

// --------------- ВРЕМЕННЫЕ ПЕРЕМЕННЫЕ ПАРСЕРА ------------------

boolean    recievedFlag;                               // буфер содержит принятые данные
boolean    parseStarted;
byte       parse_index;
String     string_convert;
String     receiveText;
boolean    haveIncomeData;
char       incomingByte;

int16_t    bufIdx = 0;                                 // Могут приниматься пакеты > 255 байт - тип int16_t
int16_t    packetSize = 0;

//*************************************************************************8

#ifdef PHTDSCONTROL
long average;                           // перем. среднего
int  PhvalArray[NUM_AVER];              // массив зачений Ph
int  TDSvalArray[NUM_AVER];             // массив зачений TDS
byte idx = 0;                           // индекс

//Заполнение массива
void ArrayFill(int newVal, int *valArray){
//  if (++idx >= NUM_AVER) idx = 0;     // перезаписывая самое старое значение
  if (idx >= NUM_AVER) idx = 0;         // перезаписывая самое старое значение
  valArray[idx] = newVal;               // пишем каждый раз в новую ячейку
}
//Вычисление среднего значения массива
int middleArifm(int *valArray) {        // принимает новое значение
  average = 0;                          // обнуляем среднее
  for (int i = 0; i < NUM_AVER; i++) {
    average += valArray[i];             // суммируем
  }
  average /= NUM_AVER;                  // делим
  return average;                       // возвращаем
}
#endif
//********************************************************************************

void(* resetFunc) (void) = 0;

// Контроль времени цикла
// uint32_t last_ms = millis();  

void process() {  

  // Время прохода одного цикла
  /*
  uint16_t duration = millis() - last_ms;
  if (duration > 0) {
    #ifdef USE_LOG
    Serial.print(F("duration="));
    Serial.println(duration);
    #endif
  }
  */

  // принимаем данные
  parsing();

#ifdef PHTDSCONTROL

  if (millis() - timing1 >=  OPROSDELAY)  // opros datchikov Ph, TDS i level
  {
    uint16_t result = 0;
    Wire.requestFrom(PHADDRESS, 2);        //requests 2 bytes
    if (Wire.available()) {
      result = Wire.read();
      result = result << 8;
      result += Wire.read();
      rawPh = result;
    }
    else rawPh = -1;
    ArrayFill(rawPh, PhvalArray);

    Wire.requestFrom(TDSADDRESS, 2);        //requests 2 bytes
    if (Wire.available()) {
      result = Wire.read();
      result = result << 8;
      result += Wire.read();
      rawTDS = result;
    }
    else rawTDS = -1;
    ArrayFill(rawTDS, TDSvalArray);
    ++idx;
    timing1 = millis();

    ioDeviceSync(ioExpInp);
    for(int i = 0; i < LVLSNSCOUNT; i++ ) {
      levels[i] = ioDeviceDigitalRead (ioExpInp, i);
    }
    for(int i = 0; i < LVLSNSCOUNT; i++ ) {
      if (levels[i] != invLVLsensor[i]){
        Wlvl = LVLSNSCOUNT - i;
        i = LVLSNSCOUNT;
      }
    }

    if (thisMode != 0 && thisMode%2 != 0 && Wlvl < 2 && !AutoFill) {                // Начало ожидания долива
      AutoFill = true;
      AutoFillTimer.reset();
    }

    if (thisMode != 0 && thisMode%2 != 0 && Wlvl >= 2 && AutoFill) {                // Сброс ожидания долива
      AutoFill = false;
    }

    if (AutoFillTimer.isReady() && thisMode != 0 && thisMode%2 != 0 && Wlvl < 2 && AutoFill){  // Старт обычного долива
      set_thisMode(thisMode + 1);
      AutoFillTimer.reset();
    } 

    if( thisMode != 0 && thisMode%2 != 0 && Wlvl < 1 ){                             // Старт экстренного долива
      set_thisMode(thisMode + 1);
      AutoFill = true;
      AutoFillTimer.reset();
    }

    if (thisMode != 0 && thisMode%2 == 0 && Wlvl >= 2 && AutoFill) {                // Окончание долива
      Serial.println("Отработка окончания долива");
      set_thisMode(thisMode - 1);
      AutoFill = false;
      AutoFillTimer.reset();
    }

    if (thisMode != 0 && thisMode%2 == 0 && levels[0] != invLVLsensor[0]){          // Обработка датчика перелива
      Serial.println("Отработка датчика перелива");
      set_thisMode(thisMode - 1);
      AutoFill = false;
      AutoFillTimer.reset();
    }
  }

  if (millis() - timing2 >  regDelay)  // Решение на регулеровку Ph
  {
    realPh = phk * middleArifm(PhvalArray) - PhMP;
    DynamicJsonDocument doc(256);
    String out;
    if ( realPh < 0 ) realPh = 0;
    if ( rawPh == -1 ) realPh = -1;
  	if (realPh > -1 && realPh <= phmin && auto_mode) {
      pumps.pourVol(phVol, PHUP);
      doc["PhUp"] = phVol;
      serializeJson(doc, out);      
      SendMQTT(out, TOPIC_REG);
      PhOk = false;
	  }

  	if (realPh > phmin && realPh < phmax) { PhOk = true; }

  	if (realPh >= phmax && auto_mode) {
      pumps.pourVol(phVol, PHDOWN);
      doc["PhDown"] = phVol;
      serializeJson(doc, out);      
      SendMQTT(out, TOPIC_REG);
      PhOk = false;
	  }
    timing3 = timing2 + ( regDelay / 2 );
    timing2 = millis();
  }

  if ( millis() - timing3  >  regDelay && (int)(millis() - timing3) > 0 )  // Решение на регулеровку TDS
  {
    realTDS = tdsk * middleArifm(TDSvalArray) - TdsMP;

    DynamicJsonDocument doc(256);
    String out;

    if ( realTDS < 0 ) realTDS = 0;
    if ( rawTDS == -1 ) realTDS = -1;

  	if (realTDS > 0 && realTDS < tdsmin && PhOk == true && auto_mode && thisMode != 0 && thisMode%2 != 0)
    {
      if(tdsAVol > 0)
      {
        pumps.pourVol(tdsAVol, TDSA);
        doc["TdsA"] = tdsAVol;
      }
      if(tdsBVol > 0)
      {
        pumps.pourVol(tdsBVol, TDSB);
        doc["TdsB"] = tdsBVol;
      }
      if(tdsCVol > 0)
      {
        pumps.pourVol(tdsCVol, TDSC);
        doc["TdsC"] = tdsCVol;
      }
      if (tdsAVol > 0 || tdsBVol > 0 || tdsCVol > 0)
      {
        serializeJson(doc, out);      
        SendMQTT(out, TOPIC_REG);
      }
	  }

  	if (realTDS > tdsmin && realTDS < tdsmax && PhOk == true && thisMode != 0 )
    {

  	}

    // Проверка на возможность разбавить расствор
  	if (realTDS > tdsmax && PhOk == true && thisMode != 0  && thisMode%2 != 0 && levels [0] == invLVLsensor[0] && auto_mode)
    {

	  }
    timing3 = millis();
  }
#endif

  if (millis() - timing >= REFRESHTIME){

    #ifdef USE_LOG

    #ifdef RTC
    Serial.print((String)getDateTimeString(rtc.now().unixtime()));
    Serial.print(", ");
    #endif

    Serial.print("UpTime:");
    Serial.print(((float)millis()/60000.0), 1);
    Serial.print(" min, "); 

    #endif  

#ifdef CO2CONTROL

    if (millis() > timing1 && millis() - timing1 >= CO2OPROSDELAY)  // opros datchika
    {
      CO2Ready = true;

///mh-z19b uart debug
     temp = co2.readCO2UART();
     if ( temp > 0 ) CO2PPM = temp;
///
      temp = co2.getLastTemperature();
      if(CO2PPM > 0){
        CO2Control(CO2PPM);
        #ifdef USE_LOG
        Serial.print("\tCO2PPM:");
        Serial.print(CO2PPM);
        Serial.print(" Temp:");
        Serial.print(temp);
        #endif      
        if (mqtt.connected()) {
          statusPub();
        }
      }
    }
#endif

#ifdef HUMCONTROL
    humd = myHumidity.readHumidity() + humcorr;
    temp = myHumidity.readTemperature() + tempcorr;

    if(humd < 998){
      if (auto_mode){
        if ( humd > maxhum ){
          digitalWrite (HUMPWR, LOW);  
        }
        if ( humd < minhum ){
          digitalWrite (HUMPWR, HIGH);  
        }
      }
      #ifdef USE_LOG
      Serial.print(" Temperature=");
      Serial.print(temp, 3);
      Serial.print(" C");
      Serial.print(" | Humidity=");
      Serial.print(humd, 3);
      Serial.print(" %");
      #endif      
      if (mqtt.connected()) {
        statusPub();
      }
    }
#endif

// I2C Scan
#if (ICCSCAN == 1)
    byte error, address;
    int nDevices;
    Serial.println("\nScanning...");
    nDevices = 0;
    for(address = 1; address < 127; address++ ) 
    {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0)
      {
        Serial.print("I2C device found at address 0x");
        if (address<16) 
        Serial.print("0");
        Serial.print(address,HEX);
        Serial.println("  !");
        nDevices++;
      }
      else if (error==4) 
      {
        Serial.print("Unknow error at address 0x");
        if (address<16) Serial.print("0");
        Serial.println(address,HEX);
      }    
    }
    if (nDevices == 0)
      Serial.println("No I2C devices found\n");
    else
      Serial.println("done");
#endif


#ifdef PHTDSCONTROL
  display.clearDisplay();
#ifdef USE_LOG
    Serial.print(" | sensor_scan >> ");
#endif
    ioDeviceSync(ioExpInp);
    for(int i = 0; i < LVLSNSCOUNT; i++ ){
      ioDeviceDigitalWrite(ioExp2, i, ioDeviceDigitalRead(ioExpInp, i));
#ifdef USE_LOG
      Serial.print(i);  
      Serial.print("-");
      Serial.print(ioDeviceDigitalRead(ioExpInp, i));
      Serial.print(" ");
#endif
    }
    ioDeviceSync(ioExp2);
#ifdef USE_LOG
    Serial.print("| ");
#endif

#ifdef DS18B20      //water temp read
    sensors.requestTemperatures(); // Send the command to get temperatures
    Wtemp = sensors.getTempCByIndex(0);
    if(Wtemp == DEVICE_DISCONNECTED_C) Serial.println("Error: Could not read temperature data");
    // else Serial.println(Wtemp);
#endif

    realPh = phk * middleArifm(PhvalArray) - PhMP;
    if ( realPh < 0 ) realPh = 0;
    realTDS = tdsk * middleArifm(TDSvalArray) - TdsMP;
    if ( realTDS < 0 ) realTDS = 0;

#ifdef USE_LOG
    Serial.print("Ph=");
    if (rawPh == -1) Serial.print("err");
    else Serial.print(realPh);
    Serial.print(" | TDS=");
    if (rawTDS == -1) Serial.print("err");
    else Serial.print(realTDS);
#ifdef DS18B20
    Serial.print(" | ");
    if(Wtemp != DEVICE_DISCONNECTED_C && Wtemp > 0) { 
      Serial.print("Water temp=");
      Serial.print(Wtemp, 2);
      Serial.print(" C");
    }
    else {
      Serial.print("Water temp=Error ");
    }
#endif
    if (rawPh != -1){
      if(RAWMode == true){
        Serial.print(" | Avg Ph RAW:");
        Serial.print(middleArifm(PhvalArray));
        Serial.print(" | ");
//      Serial.print(" Ph RAW:");
//      Serial.print(rawPh);
//      Serial.print("; ");
      }
    }
    if (rawTDS != -1) {  
      if(RAWMode == true) {
        Serial.print("| Avg TDS RAW:");
        Serial.print(middleArifm(TDSvalArray));
        Serial.print(" | ");
//      Serial.print(" TDS RAW:");
//      Serial.print(rawTDS);
//      Serial.print("; ");
      }
    }
#endif

    if (mqtt.connected()){
      statusPub();    //Публикация состояния параметров системы
    } 

#ifdef PHTDSCONTROL
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("Ph: ");
    if (rawPh == -1) display.print(String("ERR"));
    else display.print(String(realPh,3));
  
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print("TDS:");

    if (rawTDS == -1) display.print(String("ERR"));
    else display.print(String(realTDS));

    display.setTextSize(2);
    display.setCursor(0, 32);
    display.print("Wt: ");
    if(Wtemp != DEVICE_DISCONNECTED_C && Wtemp > 0) { 
      display.print(Wtemp,2);
      display.print("C");
    }
    else display.print(String("ERR"));
  
    display.setTextSize(1);
    display.setCursor(0, 57);
    display.print(WiFi.localIP());
    display.print(":");
    display.print(localPort);
    display.display();
#endif
#endif


#ifdef USE_LOG
    Serial.print("\n");
#endif




    timing = millis();
  }

  if (!parseStarted) 
  {
    // Раз в час выполнять пересканирование текстов бегущих строк на наличие события непрерывного отслеживания.
    // При сканировании события с нечеткими датами (со звездочками) просматриваются не далее чем на сутки вперед
    // События с более поздним сроком не попадут в отслеживание. Поэтому требуется периодическое перестроение списка.
    // Сканирование требуется без учета наличия соединения с интернетом и значения флага useNTP - время может быть установлено вручную с телефона

    if (wifi_connected) {

      // Если настройки программы предполагают синхронизацию с NTP сервером и время пришло - выполнить синхронизацию
      if (useNtp) {
        if ((ntp_t > 0) && getNtpInProgress && (millis() - ntp_t > 30000)) {
          #ifdef USE_LOG
          Serial.println(F("Таймаут NTP запроса!"));
          #endif
          ntp_cnt++;
          getNtpInProgress = false;
          if (init_time && ntp_cnt >= 10) {
            #ifdef USE_LOG
            Serial.println(F("Не удалось установить соединение с NTP сервером."));  
            #endif
            refresh_time = false;
          }
          
          #if (USE_MQTT == 1)
          DynamicJsonDocument doc(256);
          String out;
          doc["act"]         = F("TIME");
          doc["server_name"] = ntpServerName;
          doc["server_ip"]   = timeServerIP.toString();
          doc["result"]      = F("TIMEOUT");
          serializeJson(doc, out);      
          SendMQTT(out, TOPIC_TME);
          #endif
        }
        
        boolean timeToSync = ntpSyncTimer.isReady();
        if (timeToSync) { ntp_cnt = 0; refresh_time = true; }
        if (timeToSync || (refresh_time && (ntp_t == 0 || (millis() - ntp_t > 60000)) && (ntp_cnt < 10 || !init_time))) {
          ntp_t = millis();
          getNTP();
        }
      }
    }

/*    
    clockTicker();
    
    checkAlarmTime();
    checkAutoMode1Time();
    checkAutoMode2Time();
    checkAutoMode3Time();
    checkAutoMode4Time();

    butt.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
    // byte clicks = 0;
    // Один клик
    if (butt.isSingle()) clicks = 1;    
    // Двойной клик
    if (butt.isDouble()) clicks = 2;
    // Тройной клик
    if (butt.isTriple()) clicks = 3;
    // Четверной и более клик
    if (butt.hasClicks()) clicks = butt.getClicks();
    
    if (butt.isPress()) {
      // Состояние - кнопку нажали  
    }
    
    if (butt.isRelease()) {
      // Состояние - кнопку отпустили
      isButtonHold = false;
    }
    
    if (butt.isHolded()) {
      isButtonHold = true;
      if (globalBrightness == 255)
        brightDirection = false;
      else if (globalBrightness == 0)  
        brightDirection = true;
      else  
        brightDirection = !brightDirection;
    }

    if (clicks > 0) {
      Serial.print(F("Кнопка нажата "));  
      Serial.print(String(clicks));
      Serial.println(F(" раз"));  
    }
    // Одинарный клик - включить . выключить панель
    if (clicks == 1) {
      if (isTurnedOff) {
        // Если выключен - включить панель, восстановив эффект на котором панель была выключена
        if (saveSpecialMode && saveSpecialModeId != 0) 
          setSpecialMode(saveSpecialModeId);
        else {
          saveMode = getCurrentManualMode();
          //if (saveMode == 0) set_globalColor(0xFFFFFF);
          Serial.println(String(F("Вкл: ")) + String(saveMode));
          setManualModeTo(getAutoplay());
          setEffect(saveMode);
        }
      } else {
        // Выключить панель, запомнив текущий режим
        saveMode = thisMode;
        boolean mm = manualMode;
        // Выключить панель - черный экран
        putCurrentManualMode(saveMode);
        putAutoplay(mm);
        Serial.println(String(F("Выкл: ")) + String(saveMode));
      }
    }
    
    // Прочие клики работают только если не выключено
    if (isTurnedOff) {
        // Удержание кнопки повышает / понижает яркость панели (лампы)
        if (isButtonHold && butt.isStep()) {
          processButtonStep();
        }
    }
*/    

    // Если есть несохраненные в EEPROM данные - сохранить их
    if (saveSettingsTimer.isReady()) {
      saveSettings();
    }
  }
}

/*
void processButtonStep() {
  if (brightDirection) {
    if (globalBrightness < 10) set_globalBrightness(globalBrightness + 1);
    else if (globalBrightness < 250) set_globalBrightness(globalBrightness + 5);
    else {
      set_globalBrightness(255);
    }
  } else {
    if (globalBrightness > 15) set_globalBrightness(globalBrightness - 5);
    else if (globalBrightness > 1) set_globalBrightness(globalBrightness - 1);
    else {
      set_globalBrightness(1);
    }
  }
}
*/

// ********************* ПРИНИМАЕМ ДАННЫЕ **********************

void parsing() {
  
  // ****************** ОБРАБОТКА *****************
  String str, str1, str2;
  byte b_tmp;
  int8_t tmp_eff;
  // char c = 0;
  boolean err = false;

  /*
      ----------------------------------------------------
    1 - калибровка насосов
        $1 0 X N - налить калибровочный обьем X насосом N
        $1 1 X N - сообщить какой обьём жидкости X налил насос N действительно (измерить мензуркой)  

    2 - налить обьем X насосом N
        $2 X N - налить обьем X насосом N

    3 - Сервисные функции
        $3 1 - profpub Вывести профиль выращивания
        $3 2 - calPointPub Вывести калибровочные точки  
        $3 3 - HWprofPub Вывести аппаратные настройки

        $3 0 - Перезагрузка

    4 - Редактирование профиля регулировки
        $4 0 Х - Задать время регулирования Ph X минут (целое число) 
        $4 1 Х - Задать обьём жидкости X мл. для регулировки Ph (целое число)  
        $4 2 Х - Задать Ph max
        $4 3 Х - Задать Ph min
        $4 4 Х - Задать объём компонента A X мл. для регулировки TDS (целое число)
        $4 5 Х - Задать объём компонента B X мл. для регулировки TDS (целое число)
        $4 6 Х - Задать объём компонента C X мл. для регулировки TDS (целое число)
        $4 7 X - Задать TDS max (целое число)
        $4 8 X - Задать TDS min (целое число)
        $4 9 X - Значение текущего калибровочного раствора Ph
        $4 10 X - Значение текущего калибровочного раствора TDS
        $4 11 X - Значение minHum
        $4 12 X - Значение maxHum
        $4 13 Х - задать время начала подачи CO2      (Х=10.20  10 часов 20 мин)
        $4 14 Х - задать время прекращения подачи CO2 (Х=10.25  10 часов 25 мин)  
        $4 15   - указателя на начало массива времени CO2  (CO2Sel = 0) 

    5 - Калибровка датчиков и настройка усилителей Ph и TDS
        $5 0 Х - Включить - 1, выключить - 0 отображение сырых данных Ph и TDS
        $5 1 Х - задать phKa
        $5 2 Х - задать phKb
        $5 3 Х - задать tdsKa
        $5 4 Х - задать tdsKb
        $5 5   - задать нулевое значение датчика CO2 MH-Z19B (400 PPM)        

        Протокол связи, посылка начинается с режима. Режимы:
    6 - текст $6 N|some text, где N - назначение текста;
        1 - имя сервера NTP
        2 - SSID сети подключения
        3 - пароль для подключения к сети 
        4 - имя точки доступа
        5 - пароль к точке доступа

        7 - строка запрашиваемых параметров для процедуры getStateString(), например - "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
        8 - имя сервера MQTT
        9 - имя пользователя MQTT
       10 - пароль к MQTT-серверу
       11 - Получать DHCP IP

       13 - префикс топика сообщения к MQTT-серверу

    7 - Управление смесителем
        $7 0 Х - переключение режима работы коллектора 0-емкость подготовки 1-бак расствора 2-подлив воды в бак расствора

    11 - Настройки MQTT-канала (см. также $6 для N=8,9,10)
      - $11 1 X;   - использовать управление через MQTT сервер X; 0 - не использовать; 1 - использовать
      - $11 2 D;   - порт MQTT

      - $11 4 D;   - Задержка между последовательными обращениями к MQTT серверу
      - $11 5;     - Разорвать подключение к MQTT серверу, чтобы он иог переподключиться с новыми параметрами
      - $11 6 X;   - Флаг - отправка состояний 0 - индивидуально 1 - пакетом
      - $11 7 D;   - интервал отправки uptime на MQTT сервер в секундах или 0, если отключено

    19 - работа с настройками часов
      - $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
      - $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс

      - $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM

      - $19 17 D I; - Продолжительность отображения даты / часов (в секундах)
       
    21 - настройки подключения к сети / точке доступа
      - $21 0 X - использовать точку доступа: X=0 - не использовать X=1 - использовать
      - $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
      - $21 2; Выполнить переподключение к сети WiFi

    23 - прочие настройки
      - $23 1 ST - Сохранить EEPROM в файл    ST = 0 - внутр. файл. систему; 1 - на SD-карту
      - $23 2 ST - Загрузить EEPROM из файла  ST = 0 - внутр. файл. системы; 1 - на SD-карты
      - $23 3    - Перезагрузка
  */  

  // Если прием данных завершен и управляющая команда в intData[0] распознана
  if (recievedFlag && intData[0] > 0 && intData[0] <= 23) 
  {
    recievedFlag = false;

    switch (intData[0]) {
      // ----------------------------------------------------
      // 1 - калибровка насосов
      //   $1 0 X N - налить калибровочный обьем X насосом N
      //   $1 1 X N - сообщить какой обьём жидкости X налил насос N действительно (измерить мензуркой)  

#ifdef PHTDSCONTROL
      case 1:
        switch (intData[1]) { 
          // $1 0 X N - налить калибровочный обьем X насосом N
          case 0:
            if (intData[2] > 0 && intData[3] > 0 && intData[3] <= PUMPCOUNT){
              pumps.pourCalVol(uint16_t(intData[2]), uint8_t(intData[3]));
            }
          break;
          // $1 1 X N - сообщить какой обьём жидкости X налил насос N 
          case 1:  
            if (intData[2] > 0 && intData[3] >= 1 && intData[3] <= PUMPCOUNT){
              putPumpScl(pumps.returnScaleCalVol(uint16_t(intData[2]), uint8_t(intData[3])), (int)(intData[3]) );
            }
          break;
        }
      break;
      // ----------------------------------------------------
      // 2 - налить обьем X насосом N
      // $2 X N - налить обьем X насосом N
      case 2:
        if (intData[1] > 0 && intData[2] >= 1 && intData[2] <= PUMPCOUNT){          
           pumps.pourVol((uint16_t)(intData[1]), uint8_t(intData[2]));
        }
      break;
#endif

    //  $3 - Сервисные функции
    //     $3 1 - profpub Вывести профиль выращивания
    //     $3 2 - calPointPub Вывести калибровочные точки  
    //     $3 3 - HWprofPub Вывести аппаратные настройки

    //     $3 0 - Перезагрузка
      case 3:
        switch (intData[1]) { 
          case 1:
            profpub();
          break;
          case 2:
            calPointPub();
          break;
          case 3:
            HWprofPub();
          break;
          case 0:
            resetFunc();
          break;
        }
      break;

      // ----------------------------------------------------
      // 4 - Редактирование профиля регулировки
      //   $4 0 Х - Задать время регулирования X минут (целое число)
      //   $4 1 Х - Задать обьём жидкости X мл. для регулировки Ph (целое число)  
      //   $4 2 Х - Задать Ph max
      //   $4 3 Х - Задать Ph min  
      //   $4 4 Х - Задать объём компонента A X мл. для регулировки TDS (целое число)
      //   $4 5 Х - Задать объём компонента B X мл. для регулировки TDS (целое число)
      //   $4 6 Х - Задать объём компонента C X мл. для регулировки TDS (целое число)
      //   $4 7 X - Задать TDS max (целое число)
      //   $4 8 X - Задать TDS min (целое число)
      //   $4 9 X - Значение текущего калибровочного раствора Ph
      //   $4 10 X - Значение текущего калибровочного раствора TDS
      //   $4 11 X - Значение minHum
      //   $4 12 X - Значение maxHum
      //   $4 13 Х - задать время начала подачи CO2      (Х=10.20  10 часов 20 мин)
      //   $4 14 Х - задать время прекращения подачи CO2 (Х=10.25  10 часов 25 мин)  
      //   $4 15   - указателя на начало массива времени CO2  (CO2Sel = 0) 
      case 4:
        switch (intData[1]) { 
          // $4 0 Х - Задать время регулирования X минут (целое число)
#ifdef PHTDSCONTROL
          case 0:
            if (floatData[0] > 0){
              putregDelay((int)floatData[0]);
              regDelay = (int)floatData[0] * 1000 * 60;
              profpub();
            }
          break;
          // $4 1 Х - Задать обьём жидкости X мл. для регулировки Ph (целое число)
          case 1:  
            if (floatData[0] > 0){
              putPhVol((int)floatData[0]);
              phVol = floatData[0];
              profpub();
            }
          break;
          // $4 2 Х - Задать Ph max
          case 2:  
            if (floatData[0] > 0 && floatData[0] < 14){
              putPhmax(floatData[0]);
              phmax = floatData[0];
              profpub();
            }
          break;
          // $4 3 Х - Задать Ph min
          case 3:  
            if (floatData[0] > 0 && floatData[0] < 14){
              putPhmin(floatData[0]);
              phmin = floatData[0];
              profpub();
            }
          break;
          // $4 4 Х - Объём компонента A X мл. для регулировки TDS (целое число)
          case 4:  
            if (floatData[0] >= 0){
              putTdsAVol((int)floatData[0]);
              tdsAVol = floatData[0];
              profpub();
            }
          break;
          // $4 5 Х - Объём компонента B X мл. для регулировки TDS (целое число)
          case 5:  
            if (floatData[0] >= 0){
              putTdsBVol((int)floatData[0]);
              tdsBVol = floatData[0];
              profpub();
            }
          break;
          // $4 6 Х - Объём компонента C X мл. для регулировки TDS (целое число)
          case 6:  
            if (floatData[0] >= 0){
              putTdsCVol((int)floatData[0]);
              tdsCVol = floatData[0];
              profpub();
            }
          break;
          // $4 7 X - Задать TDS max (целое число)
          case 7:  
            if (floatData[0] > 0){
              putTDSmax((int)floatData[0]);
              tdsmax = floatData[0];
              profpub();
            }
          break;
          // $4 8 X - Задать TDS min (целое число)
          case 8:  
            if (floatData[0] >= 0){
              putTDSmin((int)floatData[0]);
              tdsmin = floatData[0];
              profpub();
            }
          break;

          // $4 9 X - Значение текущего калибровочного раствора Ph
          case 9:  
            if (floatData[0] > 0 && floatData[0] < 14){
              if (!PhCal){
                PhCalP1 = floatData[0];
                rawPhCalP1 = rawPh;
                PhCal = true;
              }
              else{
                PhCalP2 = floatData[0];
                rawPhCalP2 = rawPh;
                float tmp;
                if(PhCalP2 < PhCalP1 && rawPhCalP2 > rawPhCalP1){
                  tmp = PhCalP2;
                  PhCalP2 = PhCalP1;
                  PhCalP1 = tmp;
                }
                if(PhCalP2 < PhCalP1 && rawPhCalP2 < rawPhCalP1){
                  tmp = PhCalP2;
                  PhCalP2 = PhCalP1;
                  PhCalP1 = tmp;
                  tmp = rawPhCalP2;
                  rawPhCalP2 = rawPhCalP1;
                  rawPhCalP1 = tmp;
                }
                putPhCalP1      (PhCalP1); 
                putRawPhCalP1   (rawPhCalP1); 
                putPhCalP2      (PhCalP2 );
                putRawPhCalP2   (rawPhCalP2);
                phk = ( PhCalP2 - PhCalP1 ) / ( rawPhCalP2 - rawPhCalP1 );
                PhMP = phk * rawPhCalP1 - PhCalP1;
                PhCal = false;
                calPointPub();
              }
            }
          break;

          // $4 10 X - Значение текущего калибровочного раствора TDS
          case 10:  
            if (floatData[0] > 0){
              if (!TDScal){
                TDSCalP1 = floatData[0];
                rawTDSCalP1 = rawTDS;
                TDScal = true;
              }
              else{
                TDSCalP2 = floatData[0];
                rawTDSCalP2 = rawTDS;
                uint16_t  tmp;
                if(TDSCalP2 < TDSCalP1 && rawTDSCalP2 > rawTDSCalP1){
                  tmp = TDSCalP2;
                  TDSCalP2 = TDSCalP1;
                  TDSCalP1 = tmp;
                }
                if(TDSCalP2 < TDSCalP1 && rawTDSCalP2 < rawTDSCalP1){
                  tmp = TDSCalP2;
                  TDSCalP2 = TDSCalP1;
                  TDSCalP1 = tmp;
                  tmp = rawTDSCalP2;
                  rawTDSCalP2 = rawTDSCalP1;
                  rawTDSCalP1 = tmp;
                }
                putTDSCalP1      (TDSCalP1); 
                putRawTDSCalP1   (rawTDSCalP1); 
                putTDSCalP2      (TDSCalP2 );
                putRawTDSCalP2   (rawTDSCalP2);
                tdsk = ( TDSCalP2 - TDSCalP1 ) / ( rawTDSCalP2 - rawTDSCalP1 );
                TdsMP = tdsk * rawTDSCalP1 - TDSCalP1;
                TDScal = false;
                calPointPub();
              }
            }
          break;
#endif

#ifdef HUMCONTROL
          // $4 11 Х - Задать minHum
          case 11:  
            if (floatData[0] > 0 && floatData[0] != getMinHum()){
              minhum = floatData[0];
              putMinHum(minhum);
              profpub();
            }
          break;
          // $4 12 Х - Задать maxHum
          case 12:  
            if (floatData[0] > 0 && floatData[0] != getMaxHum()){
              maxhum = floatData[0];
              putMaxHum(maxhum);
              profpub();
            }
          break;
#endif

#ifdef CO2CONTROL
          //   $4 13 Х - задать время начала регулировки CO2      (Х=10.20  10 часов 20 мин)
          case 13:  
            {
              int CO2Temp = floatData[0]*100;
              if (CO2Sel>=0 && CO2Temp <= 0){
                CO2ON[CO2Sel] = (int)0;
                CO2OFF[CO2Sel] = (int)0;
                if (CO2Sel >= CO2_CYCLE-1) CO2Sel = 0;
                else ++CO2Sel;
                saveSettings();
              }
              else
              if (CO2Sel>=0 && CO2Check(CO2Temp)==0)
              {
                CO2ON[CO2Sel] = CO2Temp;
                if (!CO2Set) CO2Set = true;
              }
              profpub();
            }
          break;
          //   $4 14 Х - задать время прекращения регулировки CO2 (Х=10.25  10 часов 25 мин)  
          case 14: 
            { 
              int CO2Temp = floatData[0]*100;
              if (CO2Temp >= 0 && CO2Set == true && CO2Check(CO2Temp) == 0)
              {
                CO2Set = false;
                CO2OFF[CO2Sel] = CO2Temp;
                putCO2On  (CO2Sel, CO2ON [CO2Sel]);
                putCO2Off (CO2Sel, CO2OFF[CO2Sel]);
                if (CO2Sel >= CO2_CYCLE-1) CO2Sel = 0;
                else ++CO2Sel;
              }
              saveSettings();
              profpub();
            }
          break;
          //   $4 15   - указателя на начало массива времени CO2  (CO2Sel = 0) 
          case 15:  
              CO2Sel = 0;
          break;


          case 16:
              int Temp = (int)floatData[0];
              if( Temp > 0 && Temp < 10000){
                CO2PPM = Temp ;
              }
              else {
                CO2PPM = 0 ;
              }  
          break;


#endif

        }
      break;

      // ----------------------------------------------------
      // 5 - Калибровка датчиков и настройка усилителей Ph и TDS и других датчиков
      //     $5 0 Х - Включить - 1, выключить - 0 отображение сырых данных(RAW) Ph и TDS
      //     $5 1 Х - задать phKa
      //     $5 2 Х - задать phKb
      //     $5 3 Х - задать tdsKa
      //     $5 4 Х - задать tdsKb
      //     $5 5   - задать нулевое значение датчика CO2 MH-Z19B (400 PPM)
      case 5:
        switch (intData[1]) { 
#ifdef PHTDSCONTROL
          case 0:
            if(intData[2] == 1) putRAWMode(true);
            else putRAWMode(false);
            RAWMode = getRAWMode();
            profpub();
          break;

          case 1:
            if(intData[2] > 0){
              phKa = intData[2];
              Wire.beginTransmission(PHREGADR);   // transmit to device
              Wire.write(byte(0x01));             // sends instruction byte  
              Wire.write(phKa);                   // sends potentiometer value byte  
              Wire.endTransmission();             // stop transmitting
              putPhKa  (phKa);
              HWprofPub();
            }
          break;

          case 2:
            if(intData[2] > 0){
              phKb = intData[2];
              Wire.beginTransmission(PHREGADR);   // transmit to device
              Wire.write(byte(0x02));             // sends instruction byte  
              Wire.write(phKb);                   // sends potentiometer value byte  
              Wire.endTransmission();             // stop transmitting
              putPhKb  (phKb);
              HWprofPub();
            }
          break;

          case 3:
            if(intData[2] > 0){
              tdsKa = intData[2];
              Wire.beginTransmission(TDSREGADR);  // transmit to device
              Wire.write(byte(0x01));             // sends instruction byte  
              Wire.write(tdsKa);                  // sends potentiometer value byte  
              Wire.endTransmission();             // stop transmitting
              putTDSKa (tdsKa);  // усиление
              HWprofPub();
            }
          break;

          case 4:
            if(intData[2] > 0){
              tdsKb = intData[2];
              Wire.beginTransmission(TDSREGADR);  // transmit to device
              Wire.write(byte(0x02));             // sends instruction byte  
              Wire.write(tdsKb);                  // sends potentiometer value byte  
              Wire.endTransmission();             // stop transmitting
              putTDSKb (tdsKb);
              HWprofPub();
            }
          break;
#endif 
#ifdef CO2CONTROL
          case 5:
            {
              co2.calibrateZero();
              Serial.println("co2.calibrateZero()");
            }
          break;
#endif
        }
      break;


      // ----------------------------------------------------
      // 6 - прием строки: строка принимается в формате N|text, где N:
      //   0 - принятый текст бегущей строки $6 0|X|text - X - 0..9,A..Z - индекс строки
      //   1 - имя сервера NTP
      //   2 - имя сети (SSID)
      //   3 - пароль к сети
      //   4 - имя точки доступа
      //   5 - пароль точки доступа

      //   7 - строка запрашиваемых параметров для процедуры getStateString(), например - "$6 7|CE CC CO CK NC SC C1 DC DD DI NP NT NZ NS DW OF"
      //   8 - имя сервера MQTT
      //   9 - имя пользователя MQTT
      //  10 - пароль к MQTT-серверу

      //  13 - префикс топика сообщения к MQTT-серверу

      // ----------------------------------------------------

      case 6:
        b_tmp = 0;
        tmp_eff = receiveText.indexOf("|");
        if (tmp_eff > 0) {
          b_tmp = receiveText.substring(0, tmp_eff).toInt();
          str = receiveText.substring(tmp_eff+1, receiveText.length()+1);
          switch(b_tmp) {

            case 1:
              set_ntpServerName(str);
              if (wifi_connected) {
                refresh_time = true; ntp_t = 0; ntp_cnt = 0;
              }
            break;

            case 2:
              set_Ssid(str);
            break;

            case 3:
              set_pass(str);
            break;

            case 4:
              set_SoftAPName(str);
            break;

            case 5:
              set_SoftAPPass(str);
              // Передается в одном пакете - использовать SoftAP, имя точки и пароль
              // После получения пароля - перезапустить создание точки доступа
              if (useSoftAP) startSoftAP();
            break;

            case 7:
              // Запрос значений параметров, требуемых приложением вида str="CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
              // Каждый запрашиваемый приложением параметр - для заполнения соответствующего поля в приложении 
              // Передать строку для формирования, затем отправить параметры в приложение
              // if (cmdSource == UDP) {
              //   str = "$18 " + getStateString(str) + ";";
              // } else {
              //   #if (USE_MQTT == 1)
              //   // Если ключи разделены пробелом - заменить на пайпы '|'
              //   // Затем добавить в строку измененных параметров changed_keys
              //   // На следующей итерации параметры из строки changed_keys будут отправлены в канал MQTT
              //   str.replace(" ","|");
              //   int16_t pos_start = 0;
              //   int16_t pos_end = str.indexOf('|', pos_start);
              //   int16_t len = str.length();
              //   if (pos_end < 0) pos_end = len;
              //   while (pos_start < len && pos_end >= pos_start) {
              //     if (pos_end > pos_start) {      
              //       String key = str.substring(pos_start, pos_end);
              //       if (key.length() > 0) addKeyToChanged(key);
              //     }
              //     pos_start = pos_end + 1;
              //     pos_end = str.indexOf('|', pos_start);
              //     if (pos_end < 0) pos_end = len;
              //   }
              //   #endif
              // }
            break;
              
          #if (USE_MQTT == 1)
            case 8:
              set_MqttServer(str);
            break;
            case 9:
              set_MqttUser(str);
            break;
            case 10:
              set_MqttPass(str);
            break;
            case 13:
              set_MqttPrefix(str);
            break;
          #endif
          }
        }

        // При сохранении текста бегущей строки (b_tmp == 0) не нужно сразу автоматически сохранять ее в EEPROM - Сохранение будет выполнено по таймеру. 
        // При получении запроса параметров (b_tmp == 7) ничего сохранять не нужно - просто отправить требуемые параметры
        // Остальные полученные строки - сохранять сразу, ибо это настройки сети, будильники и другая критически важная информация
        switch (b_tmp) {
          case 7:
            // Ничего делать не нужно
            break;
          default:  
            saveSettings();
            break;
        }
        
        if (b_tmp >= 1 && b_tmp <= 13) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            switch (b_tmp) {

              default:
                sendAcknowledge(cmdSource);
                break;
            }
          } 
          else {
            switch (b_tmp) {
              default:
                // Другие команды - отправить подтверждение о выполнении
                sendAcknowledge(cmdSource);
                break;
            }
          }
        } 
        else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }        
        break;

      // $7 - Управление смесителем
      //   $7 0 Х - переключение режима работы коллектора 0-емкость подготовки 1-бак расствора 2-подлив воды в бак расствора
      //   $7 1 Х - вкл\выкл режима корректировки расствора 0-выкл 1-вкл
 
      case 7:
        switch (intData[1]) 
        { 
          case 0:
            if (intData[2] >= 0 && intData[2] <= MAX_EFFECT) set_thisMode(intData[2]);
          break;
          case 1:
            if (intData[2] == 0) auto_mode = false;
            else auto_mode = true;
            statusPub();
          break;
        }
      break;

      // ----------------------------------------------------
      // 11 - Настройки MQTT-канала
      // - $11 1 X;   - использовать управление через MQTT сервер X; 0 - не использовать; 1 - использовать
      // - $11 2 D;   - Порт MQTT
      // - $11 4 D;   - Задержка между последовательными обращениями к MQTT серверу
      // - $11 5;     - Разорвать подключение к MQTT серверу, чтобы он мог переподключиться с новыми параметрами
      // - $11 6 X;   - Флаг - отправка состояний 0 - индивидуально 1 - пакетом
      // - $11 7 D;   - интервал отправки uptime на MQTT сервер в секундах или 0, если отключено
      // ----------------------------------------------------

      #if (USE_MQTT == 1)
      case 11:
        switch (intData[1]) {
          case 1:               // $11 1 X; - Использовать канал MQTT: 0 - нет; 1 - да
            set_useMQTT(intData[2] == 1);
          break;
          case 2:   // $11 2 D; - Порт MQTT
            set_mqtt_port(intData[2]);
          break;
          case 4:   // $11 4 D; - Задержка между последовательными обращениями к MQTT серверу
            set_mqtt_send_delay(intData[2]);
          break;
          case 5:   // $11 5;   - Сохранить изменения ипереподключиться к MQTT серверу
            saveSettings();
            mqtt.disconnect();
            // Если подключаемся к серверу с другим именем и/или на другом порту - 
            // простой вызов 
            mqtt.setServer(mqtt_server, mqtt_port);
            // не срабатывает - соединяемся к прежнему серверу, который был обозначен при старте программы
            // Единственный вариант - программно перезагрузить контроллер. После этого новый сервер подхватывается
            if (last_mqtt_server != String(mqtt_server) || last_mqtt_port != mqtt_port) {              
              ESP.restart();
            }
          break;
          case 6:   // $11 6 X; - Отправка параметров состояния в MQTT: 0 - индивидуально; 1 - пакетом
            set_mqtt_state_packet ( intData[2] == 1 );
          break;
          case 7:   // $11 7 D; - Интервал отправки uptime на сервер MQTT в секундах
            set_upTimeSendInterval ( intData[2] );
          break;
          default:
            err = true;
            notifyUnknownCommand ( incomeBuffer );
          break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          sendAcknowledge(cmdSource);
        }
        break;
      #endif
      
      // ----------------------------------------------------
      // 19 - работа с настройками часов
      //   $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
      //   $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
      //   $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
      // ----------------------------------------------------
      
      case 19: 
        switch (intData[1]) {
          case 2:               // $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
            set_useNtp(intData[2] == 1);
            if (wifi_connected) {  refresh_time = true; ntp_t = 0; ntp_cnt = 0; }
          break;
          case 3:               // $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс            
            set_syncTimePeriod(intData[2]);
            set_timeZoneOffset((int8_t)intData[3]);
            ntpSyncTimer.setInterval ( 1000L * 60 * syncTimePeriod );
            if (wifi_connected) {  refresh_time = true; ntp_t = 0; ntp_cnt = 0;  }
          break;

#ifdef RTC
          case 8:               // $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
            setTime ( intData[5],intData[6],0,intData[4],intData[3],intData[2] );
            rtc.adjust(rtc.now().unixtime()); 
            init_time = true; refresh_time = false; ntp_cnt = 0;
            //  rescanTextEvents();
          break;
#endif
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
          break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] != 8)  sendPageParams(4, cmdSource);
            else  sendAcknowledge(cmdSource);
          } 
          else  sendAcknowledge(cmdSource);
        }
        break;

      // ----------------------------------------------------
      // 21 - настройки подключения к сети / точке доступа
      //   $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
      //   $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
      //   $21 2; Выполнить переподключение к сети WiFi
      //   $21 3 X; - Получать адрес DHCP IP; 0 - не получать; 1 - получать
      // ----------------------------------------------------

      case 21:
        // Настройки подключения к сети
        switch (intData[1]) { 
          // $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
          case 0:  
            set_useSoftAP(intData[2] == 1);
            if (useSoftAP && !ap_connected)  startSoftAP();
            else if (!useSoftAP && ap_connected) {
              if (wifi_connected) { 
                ap_connected = false;              
                WiFi.softAPdisconnect(true);
                Serial.println(F("Точка доступа отключена."));
              }
            }      
          break;
          case 1:  
            // $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к WiFi сети, пример: $21 1 192 168 0 106
            // Локальная сеть - 10.х.х.х или 172.16.х.х - 172.31.х.х или 192.168.х.х
            // Если задан адрес не локальной сети - сбросить его в 0.0.0.0, что означает получение динамического адреса 
            if (!(intData[2] == 10 || (intData[2] == 172 && intData[3] >= 16 && intData[3] <= 31) || (intData[2] == 192 && intData[3] == 168))) {
              set_StaticIP(0, 0, 0, 0);
            }
            set_StaticIP(intData[2], intData[3], intData[4], intData[5]);
          break;
          case 2:  
            // $21 2; Выполнить переподключение к сети WiFi
            saveSettings();
            delay(10);
            startWiFi(5000);     // Время ожидания подключения 5 сек
          break;
          case 3:               // $21 3 X; - Получать адрес DHCP IP; 0 - не получать; 1 - получать
            set_useDHCP(intData[2] == 1);
            Serial.print("\nuseDHCP - ");
            Serial.println(useDHCP);
          break;

          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
          break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] == 0 || intData[1] == 1) {
              sendAcknowledge(cmdSource);
            } 
            else {
              sendPageParams(6, cmdSource);
            }
          } 
          else {
            sendAcknowledge(cmdSource);
          }
        }
        break;

      // ----------------------------------------------------
      // 23 - прочие
      case 23:
        // $23 1 ST   - Сохранить EEPROM в файл    ST = 0 - внутр. файл. систему; 1 - на SD-карту
        // $23 2 ST   - Загрузить EEPROM из файла  ST = 0 - внутр. файл. системы; 1 - на SD-карты
        switch(intData[1]) {
          case 1:
           err = !saveEepromToFile(intData[2] == 1 ? "SD" : "FS");
            if (err) {
              str = F("$18 ER:[E~Не удалось сохранить резервную копию настроек]|EE:");
            } else {
              str = F("$18 ER:[I~Резервная копия настроек создана]|EE:");
            }
            str += String(eeprom_backup) + ";";
            sendStringData(str, cmdSource);
          break;
          case 2:
            err = !loadEepromFromFile(intData[2] == 1 ? "SD" : "FS");
            if (err) {
              str = F("$18 ER:[E~Не удалось загрузить резервную копию настроек];");
            } else {
              str = F("$18 ER:[I~Настройки из резервной копии восстановлены];");
            }
            sendStringData(str, cmdSource);
            // Если настройки загружены без ошибок - перезагрузить устройство
            if (!err) {
              delay(500);
              ESP.restart();
            }
          break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
          break;
        }
        break;
      // ----------------------------------------------------
      default:
        #if (USE_MQTT == 1)
        notifyUnknownCommand(incomeBuffer);
        #endif
      break;
    }
  }

  // ****************** ПАРСИНГ *****************

  // Если предыдущий буфер еще не разобран - новых данных из сокета не читаем, продолжаем разбор уже считанного буфера
  haveIncomeData = bufIdx > 0 && bufIdx < packetSize; 

  #if (USE_MQTT == 1)
  if (!haveIncomeData) {
    // Есть ли поступившие по каналу MQTT команды?
    if (queueLength > 0) {
      String command = cmdQueue[queueReadIdx++];
      if (queueReadIdx >= QSIZE_IN) queueReadIdx = 0;
      queueLength--;
      cmdSource = MQTT;
      haveIncomeData = true;
      bufIdx = 0;
      packetSize = command.length();
      memcpy(incomeBuffer, command.c_str(), packetSize);
    }
  }
  #endif
  
  if (!haveIncomeData) {
    packetSize = udp.parsePacket();      
    haveIncomeData = packetSize > 0;      
  
    if (haveIncomeData) {                
      // read the packet into packetBufffer
      int len = udp.read(incomeBuffer, BUF_MAX_SIZE);
      if (len > 0) {          
        incomeBuffer[len] = 0;
      }
      bufIdx = 0;

      cmdSource = UDP;
      
      delay(0);            // ESP8266 при вызове delay отрабатывает стек IP протокола, дадим ему поработать        

      Serial.print(F("UDP << ip='"));
      IPAddress remote = udp.remoteIP();
      Serial.print(remote.toString());
      Serial.print(":");
      Serial.print(udp.remotePort());
      Serial.print("'");
      if (udp.remotePort() == localPort) {
        Serial.print(F("; cmd='"));
        Serial.print(incomeBuffer);
        Serial.print("'");
      }
      if (udp.remotePort() == 123) {
        Serial.print(F(" - ntp sync"));
      }
//      Serial.println();

      Serial.print(F(" UDP packet "));
      Serial.print(packetSize);
      Serial.println(F(" байт"));
    }

    // NTP packet from time server
    if (haveIncomeData && udp.remotePort() == 123) {
      parseNTP();
      haveIncomeData = false;
      bufIdx = 0;      
    }
  }

  if (haveIncomeData) {         
    // Из-за ошибки в компоненте UdpSender в Thunkable - теряются половина отправленных 
    // символов, если их кодировка - двухбайтовый UTF8, т.к. оно вычисляет длину строки без учета двухбайтовости
    // Чтобы символы не терялись - при отправке строки из андроид-программы, она добивается с конца пробелами
    // Здесь эти конечные пробелы нужно предварительно удалить
    while (packetSize > 0 && incomeBuffer[packetSize-1] == ' ') packetSize--;
    incomeBuffer[packetSize] = 0;

    if (parseMode == TEXT) {                         // если нужно принять строку - принимаем всю
      // Оставшийся буфер преобразуем с строку
      if (intData[0] == 6) {  // текст
        receiveText = String(&incomeBuffer[bufIdx]);
        receiveText.trim();
      }                
      incomingByte = ending;                       // сразу завершаем парсинг
      parseMode = NORMAL;
      bufIdx = 0; 
      packetSize = 0;                              // все байты из входящего пакета обработаны
    } 

    else     
    {
      incomingByte = incomeBuffer[bufIdx++];       // обязательно ЧИТАЕМ входящий символ
    } 
  }       
    
  if (haveIncomeData) {
    if (parseStarted)                                             // если приняли начальный символ (парсинг разрешён)
    {  
      if (incomingByte != divider && incomingByte != ending)      // если это не пробел И не конец 
      {
        string_convert += incomingByte;                           // складываем в строку
      } 

      else // если это пробел или ; конец пакета
      {                                                              
        if (parse_index == 0) {
          byte cmdMode = string_convert.toInt();
          intData[0] = cmdMode;
          if (cmdMode == 6) {
            parseMode = TEXT;
          }
          if (cmdMode == 4) {
            parseMode = FRACTION;
          }
          else 
           parseMode = NORMAL;
        }
        if (parse_index == 1) {       // для второго (с нуля) символа в посылке
          if (parseMode == NORMAL) intData[parse_index] = string_convert.toInt();           // преобразуем строку в int и кладём в массив}
          if (parseMode == FRACTION) intData[parse_index] = string_convert.toInt();  
        }        
        if (parse_index == 2) {       // для третьего (с нуля) символа в посылке
          if (parseMode == NORMAL) intData[parse_index] = string_convert.toInt();  
          if (parseMode == FRACTION) 
          {
            floatData[0] = string_convert.toFloat();
            parseMode = NORMAL;
          }
        }
        else 
        {
          intData[parse_index] = string_convert.toInt();  // преобразуем строку в int и кладём в массив
        }
        string_convert = "";                        // очищаем строку
        parse_index++;                              // переходим к парсингу следующего элемента массива
      }
    }

    if (incomingByte == header) {                   // если это $
      parseStarted = true;                          // поднимаем флаг, что можно парсить
      parse_index = 0;                              // сбрасываем индекс
      string_convert = "";                          // очищаем строку
    }

    if (incomingByte == ending) {                   // если таки приняли ; - конец парсинга
      parseMode = NORMAL;
      parseStarted = false;                         // сброс
      recievedFlag = true;                          // флаг на принятие
      bufIdx = 0;
    }

    if (bufIdx >= packetSize) {                     // Весь буфер разобран 
      bufIdx = 0;
      packetSize = 0;
    }
  }
}

void sendPageParams(int page) {
  sendPageParams(page, BOTH);
}

void sendPageParams(int page, eSources src) {
  String str = "";//, color, text;

  boolean err = false;
  
  switch (page) { 
    case 1:  // Настройки
      str = getStateString("W|H|DM|PS|PD|IT|AL|RM|PW|BR|WU|WT|WR|WS|WC|WN|WZ|SD|FS|EE");
      break;
    case 4:  // Настройки часов
      str = getStateString("CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF|TM");
      break;
    case 6:  // Настройки подключения
      str = getStateString("AU|AN|AA|NW|NA|IP|QZ|QA|QP|QS|QU|QW|QD|QR|QK|UI");
      break;
     
    default:
      err = true;
      #if (USE_MQTT == 1)
      DynamicJsonDocument doc(256);
      String out;
      doc["message"] = F("unknown page");
      doc["text"]    = String(F("нет страницы с номером ")) + String(page);
      serializeJson(doc, out);      
      SendMQTT(out, TOPIC_ERR);
      #endif
      break;
  }

  if (!err) {
    if (str.length() > 0) {
      // Отправить клиенту запрошенные параметры страницы / режимов
      str = "$18 " + str + ";";
      sendStringData(str, src);
    } else {
      sendAcknowledge(cmdSource);
    }
  }
}

void sendStringData(String &str, eSources src) {
  #if (USE_MQTT == 1)
  if (src == MQTT || src == BOTH) {
    SendMQTT(str, TOPIC_DTA);
  }
  #endif
  if (src == UDP || src == BOTH) {
    int max_text_size = sizeof(incomeBuffer);        // Размер приемного буфера формирования текста загружаемой / отправляемой строки
    memset(incomeBuffer, '\0', max_text_size);
    str.toCharArray(incomeBuffer, str.length() + 1);        
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*) incomeBuffer, str.length()+1);
    udp.endPacket();
    delay(0);
    Serial.println(String(F("UDP ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(incomeBuffer));
  }
}

String getStateValue(String &key, int8_t effect, JsonVariant* value) {

  // AA:[текст]  пароль точки доступа
  // AN:[текст]  имя точки доступа
  // AT: DW HH MM  часы-минуты времени будильника для дня недели DW 1..7 -> например "AT:1 09 15"
  // AU:X        создавать точку доступа 0-нет, 1-да
  // EE:X        Наличие сохраненных настроек EEPROM на SD-карте или в файловой системе МК: 0 - нет 1 - есть в FS; 2 - есть на SD; 3 - есть в FS и на SD
  // ER:[текст]  отправка клиенту сообщения инфо/ошибки последней операции (WiFiPanel - сохр. резервной копии настроекж WiFiPlayer - сообщение операции с изображением)
  // FS:X        доступность внутренней файловой системы микроконтроллера для хранения файлов: 0 - нет, 1 - да
  // IP:xx.xx.xx.xx Текущий IP адрес WiFi соединения в сети
  // IT:число    время бездействия в секундах
  // NA:[текст]  пароль подключения к сети
  // NP:Х        использовать NTP, где Х = 0 - выкл; 1 - вкл
  // NS:[текст]  сервер NTP, ограничители [] обязательны
  // NT:число    период синхронизации NTP в минутах
  // NW:[текст]  SSID сети подключения
  // NZ:число    часовой пояс -12..+12
  // PS:X        состояние программного вкл/выкл панели 0-выкл, 1-вкл
  // QA:X        использовать MQTT 0-нет, 1-да
  // QD:число    задержка отправки сообщения MQTT
  // QP:число    порт подключения к MQTT серверу
  // QK:X        пакетная отправка состояний в MQTT-канал 0 - каждое состояние отправляется индивидуално в свой топик, соответствующий названию параметра, 1 - состояние отправляется сборными пакетами 
  // QR:X        префикс для формирования топика
  // QS:[text]   имя MQTT сервера, например QS:[srv2.clusterfly.ru]
  // QU:[text]   имя пользователя MQTT соединения, например QU:[user_af7cd12a]
  // QW:[text]   пароль MQTT соединения, например QW:[pass_eb250bf5]
  // QZ:X        сборка поддерживает MQTT 0-нет, 1-да
  // UP:число    uptime системы в секундах

  String str = "", tmp;
  
/*
  // Сработал будильник 0-нет, 1-да
  if (key == "AL") {
    if (value) {
      value->set((isAlarming) && !isAlarmStopped);
      return String((isAlarming) && !isAlarmStopped);
    }
    return str + "AL:" + String(((isAlarming) && !isAlarmStopped)); 
  }

  // Текущий эффект - id
  if (key == "EF") {
    if (value) {
      value->set(effect);
      return String(effect);
    }
    return str + "EF:" + String(effect+1); // +1 т.к эффекты считаются с нуля, а индекс в списке эффектов - с 1
  }

  // Сколько ячеек осталось свободно для хранения строк
  if (key == "OM") {
    if (value) {
      value->set(memoryAvail);
      return String(memoryAvail);
    }
    return str + "OM:" + String(memoryAvail);
  }

   // Строка состояния заполненности строк текста
   if (key == "TS") {
     tmp = getTextStates();
     if (value) {
       value->set(tmp);
       return tmp;
     }
     return str + "TS:" + tmp;
   }
*/

  // Использовать получение времени с интернета
  if (key == "NP") {
    if (value) {
      value->set(useNtp);
      return String(useNtp);
    }
    return str + "NP:" + String(useNtp);
  }

  // Период синхронизации NTP в минутах
  if (key == "NT") {
    if (value) {
      value->set(syncTimePeriod);
      return String(syncTimePeriod); 
    }
    return str + "NT:" + String(syncTimePeriod); 
  }

  // Часовой пояс
  if (key == "NZ") {
    if (value) {
      value->set(timeZoneOffset);
      return String(timeZoneOffset);
    }
    return str + "NZ:" + String(timeZoneOffset);
  }

  // Имя сервера NTP (url)
  if (key == "NS") {
    tmp = String(ntpServerName);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "NS:" + "[" + tmp + "]";
  }

  // uptime - время работы системы в секундах
  if (key == "UP") {
    uint32_t upt = upTime;
    if (upt > 0) {
      upt = ((uint32_t)now()) - upt;
    }
    if (value) {
      value->set(upt);
      return String(upt);  
    }
    return str + "UP:" + String(upt);
  }

  // создавать точку доступа
  if (key == "AU") {
    if (value) {
      value->set(useSoftAP);
      return String(useSoftAP);  
    }
    return str + "AU:" + String(useSoftAP);
  }

  // Имя точки доступа
  if (key == "AN") {
    tmp = String(apName);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AN:" + "[" + tmp + "]";
  }

  // Пароль точки доступа
  if (key == "AA") {
    tmp = String(apPass);
    if (value) {
      // value->set(tmp);  // Решено пароль по открытому каналу на [публичный] сервер MQTT не отправлять
      return "";           // tmp;
    }
    return str + "AA:" + "[" + tmp + "]";
  }

  // Имя локальной сети (SSID)
  if (key == "NW") {
    tmp = String(ssid);
    if (value){
      value->set(tmp);
      return tmp;
    }
    return str + "NW:" + "[" + tmp + "]";
  }

  // Пароль к сети
  if (key == "NA") {
    tmp = String(pass);
    if (value) {
      // value->set(tmp);  // Решено пароль по открытому каналу на [публичный] сервер MQTT не отправлять
      return "";           // tmp;
    }
    return str + "NA:" + "[" + tmp + "]";
  }

  // IP адрес
  if (key == "IP") {
    tmp = String(wifi_connected ? WiFi.localIP().toString() : "");
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "IP:" + tmp;
  }
/*
  // Время Режима №1
  if (key == "AM1T") {
    tmp = padNum(AM1_hour,2) + " " + padNum(AM1_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM1T:" + tmp;
  }

  // Действие Режима №1
  if (key == "AM1A") {
    if (value) {
      value->set(AM1_effect_id);
      return String(AM1_effect_id);
    }
    return str + "AM1A:" + String(AM1_effect_id);
  }

  // Время Режима №2
  if (key == "AM2T") {
    tmp = padNum(AM2_hour,2) + " " + padNum(AM2_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM2T:" + tmp;
  }

  // Действие Режима №2
  if (key == "AM2A") {
    if (value) {
      value->set(AM2_effect_id);
      return String(AM2_effect_id);
    }
    return str + "AM2A:" + String(AM2_effect_id);
  }

  // Время Режима №3
  if (key == "AM3T") {
    tmp = padNum(AM3_hour,2) + " " + padNum(AM3_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM3T:" + tmp;
  }

  // Действие Режима №3
  if (key == "AM3A") {
    if (value) {
      value->set(AM3_effect_id);
      return String(AM3_effect_id);
    }
    return str + "AM3A:" + String(AM3_effect_id);
  }

  // Время Режима №4
  if (key == "AM4T") {
    tmp = padNum(AM4_hour,2) + " " + padNum(AM4_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM4T:" + tmp;
  }

  // Действие Режима №4
  if (key == "AM4A") {
    if (value) {
      value->set(AM4_effect_id);
      return String(AM4_effect_id);
    }
    return str + "AM4A:" + String(AM4_effect_id);
  }

  // Действие Режима "Рассвет"
  if (key == "AM5A") {
    if (value) {
      value->set(dawn_effect_id);
      return String(dawn_effect_id);
    }
    return str + "AM5A:" + String(dawn_effect_id);
  }

  // Действие Режима "Закат"
  if (key == "AM6A") {
    if (value) {
      value->set(dusk_effect_id);
      return String(dusk_effect_id);
    }
    return str + "AM6A:" + String(dusk_effect_id);
  }
*/
  // Наличие резервной копии EEPROM
  if (key == "EE") {
    if (value) {
      value->set(eeprom_backup);
      return String(eeprom_backup);
    }
    return str + "EE:" + String(eeprom_backup); 
  }

  // Доступность внутренней файловой системы
  if (key == "FS") {
    if (value) {
      value->set(spiffs_ok);
      return String(spiffs_ok);
    }
    return str + "FS:" + String(spiffs_ok); 
  }

  // Прошивка поддерживает MQTT 0-нет, 1-да
  if (key == "QZ") {
    if (value) {
      value->set(USE_MQTT == 1);
      return String(USE_MQTT == 1);
    }
    return str + "QZ:" + String(USE_MQTT == 1);  
  }

#if (USE_MQTT == 1)

  // Использовать MQTT 0-нет, 1-да
  if (key == "QA") {
    if (value) {
      value->set(useMQTT);
      return String(useMQTT);  
    }
    return str + "QA:" + String(useMQTT);  
  }

  // QP:число    порт подключения к MQTT серверу
  if (key == "QP") {
    if (value) {
      value->set(mqtt_port);
      return String(mqtt_port);  
    }
    return str + "QP:" + String(mqtt_port);  
  }

  // QS:[text]   имя MQTT сервера, например QS:[srv2.clusterfly.ru]
  if (key == "QS") {
    tmp = String(mqtt_server);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "QS:" + "[" + tmp +  "]";
  }
  
  // QU:[text]   имя пользователя MQTT соединения, например QU:[user_af7cd12a]
  if (key == "QU") {
    tmp = String(mqtt_user);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "QU:" + "[" + tmp +  "]";
  }

  // QW:[text]   пароль MQTT соединения, например QW:[pass_eb250bf5]
  if (key == "QW") {
    tmp = String(mqtt_pass);
    if (value) {
      // value->set(tmp);  // Решено пароль по открытому каналу на [публичный] сервер MQTT не отправлять
      return "";           // tmp;
    }
    return str + "QW:" + "[" + tmp +  "]";
  }

  // QD:число    задержка между отправками сообщений к MQTT серверу
  if (key == "QD") {
    if (value) {
      value->set(mqtt_send_delay);
      return String(mqtt_send_delay);
    }
    return str + "QD:" + String(mqtt_send_delay);  
  }

  // QR:[text]   префикс топика MQTT сообщения, например QR:[user_af7cd12a/WiFiPanel-0]
  if (key == "QR") {
    tmp = String(mqtt_prefix);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "QR:" + "[" + tmp +  "]";
  }

  // Использовать отправку состояния параметров в MQTT пакетами 0-индивидуально, 1-пакетами
  if (key == "QK") {
    if (value) {
      value->set(mqtt_state_packet);
      return String(mqtt_state_packet);  
    }
    return str + "QK:" + String(mqtt_state_packet);
  }

  // Интервал отправки uptime на cервер в секундах
  if (key == "UI") {
    if (value) {
      value->set(upTimeSendInterval);
      return String(upTimeSendInterval);  
    }
    return str + "UI:" + String(upTimeSendInterval);
  }
#endif

  // Запрошенный ключ не найден - вернуть пустую строку
  return "";
}

String getStateString(String keys) {
  String str = "", s_tmp, key;

  // Ключи буквы/цифры, разделенные пробелами или пайпами '|' 
  // Если строка ключей ограничена квадратными скобками или кавычками - удалить их;
  // В конце может быть ";" - не требуется - удалить ее (в середине ее быть не может)
  keys.replace(";","");
  keys.replace("[","");
  keys.replace("]","");
  keys.replace("\"","");

  // Ключи могут быть разделены '|' или пробелами
  keys.replace(" ","|");
  keys.replace("/r"," ");
  keys.replace("/n"," ");
  keys.trim();
  
  int8_t tmp_eff = -1;
  int16_t pos_start = 0;
  int16_t pos_end = keys.indexOf('|', pos_start);
  int16_t len = keys.length();
  if (pos_end < 0) pos_end = len;


  // Строка keys содержит ключи запрашиваемых данных, разделенных знаком '|', например "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
  while (pos_start < len && pos_end >= pos_start) {
    if (pos_end > pos_start) {      
      key = keys.substring(pos_start, pos_end);
      if (key.length() > 0) {
        s_tmp = getStateValue(key, tmp_eff);
        if (s_tmp.length() > 0) {
          str += s_tmp + "|";
        }
      }      
    }
    pos_start = pos_end + 1;
    pos_end = keys.indexOf('|', pos_start);
    if (pos_end < 0) pos_end = len;
  }

  len = str.length();
  if (len > 0 && str.charAt(len - 1) == '|') {
    str = str.substring(0, len - 1);
  }
  return str;
}

void sendAcknowledge(eSources src) {
  if (src == UDP || src == BOTH) {
    // Отправить подтверждение, чтобы клиентский сокет прервал ожидание
    String reply = "";
    boolean isCmd = false; 
    if (cmd95.length() > 0) { reply += cmd95; cmd95 = ""; isCmd = true;}
    if (cmd96.length() > 0) { reply += cmd96; cmd96 = ""; isCmd = true; }
    reply += "ack" + String(ackCounter++) + ";";  
    reply.toCharArray(replyBuffer, reply.length()+1);
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*) replyBuffer, reply.length()+1);
    udp.endPacket();
    delay(0);
    if (isCmd) {
      Serial.println(String(F("Ответ на ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(replyBuffer));
    }
  }
}