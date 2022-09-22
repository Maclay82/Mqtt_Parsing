#ifndef DEF_HARD_H
#define DEF_HARD_H
#endif

#define EEPROM_OK     0xA2       // Флаг, показывающий, что EEPROM инициализирована корректными данными 
#define EEPROM_MAX    4096       // Максимальный размер EEPROM доступный для использования
#define EFFECT_EEPROM  500       // начальная ячейка eeprom с параметрами эффектов, 5 байт на эффект

// *************************************************************************
enum  eModes   {NORMAL, FRACTION, TEXT};
enum  eSources {NONE, BOTH, UDP, MQTT};

// ****************** ПРОФИЛИ УСТРОЙСТВ *******************

// ВНИМАНИЕ!!! 

// Для плат Wemos D1 mini / Wemos D1 pro в настройках компиляции выбирайте "NodeMCU 1.0(ESP-12E Module)"
// --------------------------------------------------------
// При использовании платы микроконтроллера Wemos D1 (xxxx) и выбранной в менеджере плат "Wemos D1 (xxxx)"
// прошивка скорее всего нормально работать не будет. 
// Наблюдались следующие сбои у разных пользователей:
// - нестабильная работа WiFi (постоянные отваливания и пропадание сети) - попробуйте варианты с разным значением параметров компиляции IwIP в Arduino IDE
// - прекращение вывода контрольной информации в Serial.print()
// - настройки в EEPROM не сохраняются
// Думаю все эти проблемы из-за некорректной работы ядра ESP8266 для платы (варианта компиляции) Wemos D1 (xxxx) в версии ядра ESP8266
// --------------------------------------------------------
//
// Если вы собираетесь испорльзовать возможность сохранения нарисованных в WiFiPlayer картинок в файловой системе микроконтроллера,
// в меню "Инструменты" Arduino IDE в настройке распределения памяти устройства выберите вариант:
//   Для микроконтроллеров ESP8266 с 4МБ флэш-памяти рекомендуется вариант "Flash Size: 4MB(FS:2MB OTA:~1019KB)"
//   Для микроконтроллеров ESP32   с 4МБ флэш-памяти рекомендуется вариант "Partition scheme: Default 4MB with spiff(1.2MB APP/1.5MB SPIFFS)"; 
// Также для ESP32 требуется дополнительно установить библиотеку 'LittleFS_esp32'
//
// --------------------------------------------------------
// Текущая версия ядра ESP32 не видит SD-карты более 4GB
// --------------------------------------------------------
//
// Ниже в этом файле размещены профили устройств, реально существующие в природе, 
// которые поддерживаются автром - прошивка в них периодически обновлется по мере развития проекта.
//
// В вашем проекте на ESP8266 рекомендуется использовать DEVICE_ID 0 и в этом профиле указать параметры ВАШЕГО проекта в блоке #if (DEVICE_ID == 0) ... #endif
// Если вы используете микроконтроллер ESP32 - используйте DEVICE_ID 0 и определяйте параметры ВАШЕГО проекта в блоке строк 383-413 внутри #if defined(ESP32) ... #endif
//
// --------------------------------------------------------


// Профиль устройства, под которое выполняется компиляция и сборка проекта

#define DEVICE_ID 4                 // 0 - Увлажнитель тестовый стенд
                                    // 1 - Увлажнитель Зеленка
                                    // 2 - Увлажнитель Перцы
                                    // 3 - PhTDS контроллер тестовый
                                    // 4 - PhTDS контроллер Зеленка lolin
                                    // 5 - Контроллер приточка Зеленка
                                    // 6 - Контроллер CO2 Зеленка
                                    // 7 - Контроллер CO2 Зеленка тест

// ================== Увлажнитель тестовый стенд =====================

#if (DEVICE_ID == 0)
/*
 * Wemos D1 mini
 * В менеджере плат выбрано NodeMCU v1.0 (ESP-12E)
 */
//#if defined(ESP8266)
#ifndef HUMCONTROL
#define HUMCONTROL
#endif
#define DEV_ID 0
#define REFRESHTIME 10000

#define HOST_NAME   F("HumCtrlTest")

#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define DEFAULT_MQTT_PREFIX "ghTest"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)
                              // 0 - Настройки MQTT и API KEY OpenWeatherMap в скетче в def_soft.h в строках: (пароли и ключи доступа определены в тексте скетча)
                              // Файл a_def_pass.h в комплект не входит, нужно создать, скопировать туда указанные строки

#define minhumDEF 69
#define maxhumDEF 74


#endif

// ================== Увлажнитель Зеленка =====================
#if (DEVICE_ID == 1)
#ifndef HUMCONTROL
#define HUMCONTROL
#endif
#define DEV_ID 0

#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define HOST_NAME   F("HumCtrl")
#define DEFAULT_MQTT_PREFIX "gh1"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)

#define REFRESHTIME 5000
#define minhumDEF 69
#define maxhumDEF 74

#define USEDHCP 1
//#define DEFAULT_IP {192, 168, 2, 162}       // Сетевой адрес устройства по умолчанию

#define ICCSCAN 0

#endif

// ================== Увлажнитель Перцы =====================
#if (DEVICE_ID == 2)
#ifndef HUMCONTROL
#define HUMCONTROL
#endif
#define DEV_ID 0
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define HOST_NAME   F("HumCtrl")
#define DEFAULT_MQTT_PREFIX "gh2"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)

#define REFRESHTIME 5000
#define minhumDEF 57
#define maxhumDEF 63

#endif


// ================== PhTDS контроллер тестовый =====================
#if (DEVICE_ID == 3)

/*
I2C address 0x2C Ph adj
I2C address 0x2E TDS adj
I2C address 0x48 Ph
I2C address 0x49 TDS

1 - V+ кр
2 - temp data син
3 - V- Gnd 
4 - tds elec. 1 зел
5 - tds elec. 2 жел

*/
#ifndef DS18B20
#define DS18B20
#endif

#ifndef PHTDSCONTROL
#define PHTDSCONTROL
#endif

#define DEV_ID 0

#define USE_MQTT            1             // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define HOST_NAME           "PhTDSCtrl"
#define DEFAULT_MQTT_PREFIX "ghTest"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS          0             // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)

#define REFRESHTIME 10000                 // Время обновления показаний прибора для MQTT

#define USEDHCP  1
//#define DEFAULT_IP {192, 168, 2, 111}       // Сетевой адрес устройства по умолчанию

#define ICCSCAN 0

#endif

// ================== PhTDS контроллер Зеленка =====================
#if (DEVICE_ID == 4)
#ifndef PHTDSCONTROL
#define PHTDSCONTROL
#endif

#ifndef lolin32
#define lolin32
#endif

#ifndef DS18B20
#define DS18B20
#endif

#define DEV_ID 4
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define HOST_NAME   F("PhTDSCtrl")
#define DEFAULT_MQTT_PREFIX "gh1"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)

#define REFRESHTIME 15000
#define USEDHCP 1
//#define DEFAULT_IP {192, 168, 1, 112}       // Сетевой адрес устройства по умолчанию

#define ICCSCAN 1
#endif

// ================== Контроллер приточка Зеленка =====================
#if (DEVICE_ID == 5)

#define DEV_ID 5
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define HOST_NAME   F("VentCtrl")
#define DEFAULT_MQTT_PREFIX "gh1"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)

#define REFRESHTIME 15000
#define USEDHCP 0
#define DEFAULT_IP {192, 168, 1, 112}       // Сетевой адрес устройства по умолчанию

#define ICCSCAN 0

#ifndef VENTCONTROL
#define VENTCONTROL
#endif

#define minhumDEF 69
#define maxhumDEF 74

#endif

// ================== Контроллер CO2 Зеленка =====================
#if (DEVICE_ID == 6)

#ifndef CO2CONTROL
#define CO2CONTROL
#endif

#define RTC

#define DEV_ID 0
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define HOST_NAME   F("CO2Ctrl")
#define DEFAULT_MQTT_PREFIX "gh1"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)

#define REFRESHTIME 30000
#define USEDHCP 1
//#define DEFAULT_IP {192, 168, 2, 162}       // Сетевой адрес устройства по умолчанию

#define ICCSCAN 0

//0x68 ds3231
//0x38 atn10

#endif

// ================== Контроллер CO2 Зеленка тест =====================
#if (DEVICE_ID == 7)

#ifndef CO2CONTROL
#define CO2CONTROL
#endif

#define RTC

#define DEV_ID 1
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 
#define HOST_NAME   F("CO2Ctrl")
#define DEFAULT_MQTT_PREFIX "gh0"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)

#define REFRESHTIME 30000
#define USEDHCP 1
//#define DEFAULT_IP {192, 168, 2, 162}       // Сетевой адрес устройства по умолчанию

#define ICCSCAN 0

//0x68 ds3231
//0x38 atn10

#endif

// =======================================================

// *************************** ПОДКЛЮЧЕНИЕ К СЕТИ **************************

// Внимание!!! Если вы меняете эти значения ПОСЛЕ того, как прошивка уже хотя бы раз была загружена в плату и выполнялась,
// чтобы изменения вступили в силу нужно также изменить значение константы EEPROM_OK в первой строке в файле eeprom.ino 

#ifndef DEFAULT_NTP_SERVER
#define DEFAULT_NTP_SERVER "ru.pool.ntp.org"                    // NTP сервер по умолчанию "time.nist.gov"
#endif

#ifndef DEFAULT_AP_NAME
#define DEFAULT_AP_NAME     "StartAP"                           // Имя точки доступа по умолчанию 
#endif

#ifndef DEFAULT_AP_PASS
#define DEFAULT_AP_PASS     "12341111"                          // Пароль точки доступа по умолчанию
#endif

#ifndef NETWORK_SSID
#define NETWORK_SSID        "OstrovDushi"//"TechNet1"//         // Имя WiFi сети
#endif

#ifndef NETWORK_PASS
#define NETWORK_PASS        "LaIslaBonita"//"fuhtufnec"//       // Пароль для подключения к WiFi сети
#endif

#ifndef DEFAULT_IP
#define DEFAULT_IP          {192, 168, 1, 121}                  // Сетевой адрес устройства по умолчанию
#endif

#ifndef USEDHCP
#define USEDHCP 1
#endif

// ************** ИСПОЛЬЗУЕМЫЕ БИБЛИОТЕКИ ****************
#include <Wire.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
#endif
#if defined(ESP32)
  #include <ESPmDNS.h>
#endif

#if (USE_MQTT == 1)
#include <PubSubClient.h>        // Библиотека для работы с MQTT
#endif

#include <ArduinoOTA.h>          // Библиотека обновления "по воздуху"
#include <WiFiUdp.h>             // Библиотека поддержки WiFi
#include <TimeLib.h>             // Библиотека поддержки функций времени
#include <EEPROM.h>              // Библиотека поддержки постоянной памяти
#include <ArduinoJson.h>         // Библиотека для работы с JSON (mqtt, состояние системы)

#ifdef DS18B20
#include <OneWire.h>             // Библиотека работы с датчиком температуры DS18B20 
#include <DallasTemperature.h>   // Библиотека работы с датчиком температуры DS18B20 
#endif

#ifdef PHTDSCONTROL
//#include <DallasTemperature.h>   // Библиотека работы с датчиком температуры DS18B20 
#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include "i2cPumps.h"
#endif

#ifdef RTC
#include "RTClib.h"
#endif

#ifdef CO2CONTROL                // CO2 PPM MH-Z19B lib
#include <SoftwareSerial.h>
#include <MHZ.h>
#endif

#ifdef HUMCONTROL                // Hum lib
#include "SparkFunHTU21D.h"
#endif

#include "timerMinim.h"          // Библиотека таймеров
#include "a_main.h"     
#include "eeprom1.h"             // Библиотека для работы с постоянной памятью
#include "mqtt.h"
#include "utility.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "FS.h"                  // Работа с внутренней файловой системой чипа ESP8266/ESP32
#if defined(ESP32)
  #define  LittleFS LITTLEFS
  #include <LITTLEFS.h>
#else
  #include <LittleFS.h>
#endif

// =======================================================

#if A_DEF_PASS == 1         
#include "a_def_pass.h"     // Если здесь ошибка - смотри комментарий к определению A_DEF_PASS выше в блоке с соответствующим DEVICE_ID
#endif

//Create an instance of the object
#ifdef RTC
  extern RTC_DS3231 rtc;
#endif

#ifdef HUMCONTROL
#if defined(ESP8266)
  #define HUMPWR D7
#endif
#if defined(ESP32)
  #define HUMPWR 23
#endif
#ifndef maxhumDEF
  #define maxhumDEF 74
#endif
#ifndef minhumDEF
  #define minhumDEF 69
#endif
extern HTU21D myHumidity;
extern float temp, humd;
#endif                                           

#ifdef CO2CONTROL                // CO2 PPM MH-Z19B pin for uart reading

#if defined(ESP8266)
  #define MH_Z19_RX D3
  #define MH_Z19_TX D4
#endif
#if defined(ESP32)
  #if defined(lolin32)
    #define MH_Z19_RX 12
    #define MH_Z19_TX 11
  #else
    #define MH_Z19_RX 17
    #define MH_Z19_TX 16
  #endif
#endif

#ifndef maxCO2DEF
  #define maxCO2DEF 1400
#endif
#ifndef minCO2DEF
  #define minCO2DEF 800
#endif

#ifndef CO2PWR
#if defined(ESP8266)
  #define CO2PWR D5
#endif
#if defined(ESP32)
  // #if defined(lolin32)
  //   #define CO2PWR 13
  // #else
    #define CO2PWR 26
//#endif
#endif
#endif

#endif

#ifdef PHTDSCONTROL
extern i2cPumps pumps;
extern float realTDS, realPh, Wtemp;
extern int Wlvl;

#define PUMPSCALEADR 400    // start pumps scale address 
#define PUMPCALVOLADR 350
extern IoAbstractionRef I2CExp, ioExp2, ioExpInp; //классы плат I2C расширителей


#define PHUP            1   //  PH up pump
#define PHDOWN          2   //  PH down pump
#define TDSA            3   //  TDS A pump
#define TDSB            4   //  TDS B pump
#define TDSC            5   //  TDS C pump
#define ADD             6   //  Addition pump

#define ClWaterIn       7   //  Вход чистой воды
#define ClWaterOut      6   //  Выход чистой воды  
#define SolWaterIn_1    5   //  Вход бака расствора 1
#define SolWaterOut_1   4   //  Выход бака расствора 1  

#define PHREGADR     0x2C   //  PH reg. AD5282 address in 7bit format
#define TDSREGADR    0x2E   // TDS reg. AD5282 address in 7bit format
#define PHADDRESS    0x48   //  PH ADC MCP3221 address in 7bit format
#define TDSADDRESS   0x49   // TDS ADC MCP3221 address in 7bit format

#ifndef LVLSNSCOUNT 
#define LVLSNSCOUNT 3       //количество датчиков уровня в ёмкости
#endif

#ifndef DEV_ID
#define DEV_ID 1
#endif

#endif

#ifndef ICCSCAN
#define ICCSCAN 0
#endif