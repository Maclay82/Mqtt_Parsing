#ifndef DEF_HARD_H
#define DEF_HARD_H

#define EEPROM_OK     0xA2      // Флаг, показывающий, что EEPROM инициализирована корректными данными 
#define EEPROM_MAX    1024       // Максимальный размер EEPROM доступный для использования
// #define EEPROM_MAX    4096       // Максимальный размер EEPROM доступный для использования
#define EFFECT_EEPROM  300       // начальная ячейка eeprom с параметрами эффектов, 5 байт на эффект

// *************************************************************************

enum  eModes   {NORMAL};
enum  eSources {NONE, BOTH, UDP, MQTT};

// *************************** ПОДКЛЮЧЕНИЕ К СЕТИ **************************

// Внимание!!! Если вы меняете эти значения ПОСЛЕ того, как прошивка уже хотя бы раз была загружена в плату и выполнялась,
// чтобы изменения вступили в силу нужно также изменить значение константы EEPROM_OK в первой строке в файле eeprom.ino 

#ifndef DEFAULT_NTP_SERVER
#define DEFAULT_NTP_SERVER "ru.pool.ntp.org" // NTP сервер по умолчанию "time.nist.gov"
#endif

#ifndef DEFAULT_AP_NAME
#define DEFAULT_AP_NAME "StartAP"           // Имя точки доступа по умолчанию 
#endif

#ifndef DEFAULT_AP_PASS
#define DEFAULT_AP_PASS "12341111"          // Пароль точки доступа по умолчанию
#endif

#ifndef NETWORK_SSID
#define NETWORK_SSID "yougrow"                     // Имя WiFi сети
#endif

#ifndef NETWORK_PASS
#define NETWORK_PASS "00007777"                     // Пароль для подключения к WiFi сети
#endif

#ifndef DEFAULT_IP
#define DEFAULT_IP {192, 168, 1, 100}       // Сетевой адрес устройства по умолчанию
#endif


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
#define HOST_NAME   F("Main")

#define DEVICE_ID   0               // 0 - Увлажнитель тестовый стенд
                                    // 1 - Увлажнитель Зеленка
                                    // 2 - Увлажнитель Перцы

// ================== Тестовый стенд =====================

#if (DEVICE_ID == 0)
/*
 * Wemos D1 mini
 * В менеджере плат выбрано NodeMCU v1.0 (ESP-12E)
 */
//#if defined(ESP8266)
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 

#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)
                              // 0 - Настройки MQTT и API KEY OpenWeatherMap в скетче в def_soft.h в строках: (пароли и ключи доступа определены в тексте скетча)
                              // Файл a_def_pass.h в комплект не входит, нужно создать, скопировать туда указанные строки
#define HUMPWR D6
#define REFRESHTIME 5000
#define MEMFLAG 0

#define minhumDEF 69
#define maxhumDEF 74

//#endif
// Update these with values suitable for your network.

#define mqttClient "GHTest"
#define mqtt_topic_temp "/ghTest/humCtrl/temp"
#define mqtt_topic_hum "/ghTest/humCtrl/hum"
#define mqtt_topic_max_hum "/ghTest/humCtrl/maxhum" // MQTT Topic
#define mqtt_topic_min_hum "/ghTest/humCtrl/minhum" // MQTT Topic
#define mqtt_topic_relay "/ghTest/humCtrl/relay" // MQTT Topic
#define mqtt_topic_com "/ghTest/humCtrl/cmd" // MQTT Топик - получение команды управления от клиента
#define mqtt_topic_stat "/ghTest/humCtrl/stt" // MQTT Топик - отправка клиенту сообщений о текущем статусе параметров устройства - основной набор параеметров (пакет)

#endif

#if (DEVICE_ID == 1)
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 

#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)
#define HUMPWR D6
#define REFRESHTIME 5000
#define MEMFLAG 0

#define minhumDEF 69
#define maxhumDEF 74

#define mqttClient F("GH1_Hum_cli1")
//const char *mqtt_client = "GH1_Hum_cli1";
const char *mqtt_topic_temp = "/gh1/humCtrl/temp"; // MQTT Topic
const char *mqtt_topic_hum = "/gh1/humCtrl/hum"; // MQTT Topic
const char *mqtt_topic_max_hum = "/gh1/humCtrl/maxhum"; // MQTT Topic
const char *mqtt_topic_min_hum = "/gh1/humCtrl/minhum"; // MQTT Topic
//const char *mqtt_topic_hum_on = "/gh1/humCtrl/on"; // MQTT Topic
const char *mqtt_topic_relay = "/gh1/humCtrl/relay"; // MQTT Topic
const char *mqtt_topic_com = "/gh1/humCtrl/cmd"; // MQTT Топик - получение команды управления от клиента
const char *mqtt_topic_stat = "/gh1/humCtrl/stt"; // MQTT Топик - отправка клиенту сообщений о текущем статусе параметров устройства - основной набор параеметров (пакет)
#endif

#if (DEVICE_ID == 2)
#define USE_MQTT 1            // 1 - использовать управление по MQTT-каналу; 0 - не использовать 

#define A_DEF_PASS 0          // 1 - Настройки MQTT и API KEY OpenWeatherMap в отдельном файле a_def_pass.h     (пароли и ключи доступа как приватные данные в отдельном файле)
 
#define mqttClient F("GH2HumControl000")
//const char *mqtt_client = "GH2HumControl000";
const char *mqtt_topic_temp = "/gh2_/humCtrl/temp"; // MQTT Topic
const char *mqtt_topic_hum = "/gh2_/humCtrl/hum"; // MQTT Topic
const char *mqtt_topic_max_hum = "/gh2_/humCtrl/maxhum"; // MQTT Topic
const char *mqtt_topic_min_hum = "/gh2_/humCtrl/minhum"; // MQTT Topic
//const char *mqtt_topic_hum_on = "/gh2_/humCtrl/on"; // MQTT Topic
const char *mqtt_topic_relay = "/gh2_/humCtrl/relay"; // MQTT Topic
const char *mqtt_topic_com = "/gh2/humCtrl/cmd"; // MQTT Топик - получение команды управления от клиента
const char *mqtt_topic_stat = "/gh2/humCtrl/stt"; // MQTT Топик - отправка клиенту сообщений о текущем статусе параметров устройства - основной набор параеметров (пакет)
#endif

// =======================================================

// ************** ИСПОЛЬЗУЕМЫЕ БИБЛИОТЕКИ ****************
#include <Wire.h>
#include <SparkFunHTU21D.h>

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
#include "timerMinim.h"          // Библиотека таймеров
#include "a_main.h"     
#include "eeprom1.h"             // Библиотека для работы с постоянной памятью
#include "mqtt.h"
#include "setters.h"


#include "FS.h"                  // Работа с внутренней файловой системой чипа ESP8266/ESP32
#if defined(ESP32)
  #define   LittleFS LITTLEFS
  #include <LITTLEFS.h>
#else
  #include <LittleFS.h>
#endif

// =======================================================

#if A_DEF_PASS == 1         
#include "a_def_pass.h"     // Если здесь ошибка - смотри комментарий к определению A_DEF_PASS выше в блоке с соответствующим DEVICE_ID
#endif

//Create an instance of the object
static HTU21D myHumidity;

#endif