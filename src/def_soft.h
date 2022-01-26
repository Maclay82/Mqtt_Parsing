#ifndef DEF_SOFT_H
#define DEF_SOFT_H
#include <def_hard.h> // Определение параметров подключения и т.п

#define USE_LOG
// Определения программных констант и переменных

#define MAX_EFFECT              2         // количество режимов работы прибора определенных в прошивке

extern unsigned long timing, timing1, timing2, timing3, per, regDelay; // Таймеры опросов

#ifdef HUMCONTROL
extern float minhum, maxhum;
#endif

#ifdef PHTDSCONTROL
#define OPROSDELAY 150        // время опроса Ph TDS в миллисекундах
#define REGDELAY 1            // время цикла регулировки в минутах
#define NUM_AVER 20           // выборка (из скольки усредняем)
extern boolean     count_mode;         // Флаг автоматического режима
extern Adafruit_SSD1306 display;
extern DallasTemperature sensors;
#endif

extern uint16_t AUTO_MODE_PERIOD;  // Период активации автоматического режима в минутах по умолчанию
extern uint16_t AUTO_FILL_PERIOD;  // Период активации автоматического подлива в часах
extern boolean     auto_mode;         // Флаг автоматического режима

// *************************** ПОДКЛЮЧЕНИЕ К СЕТИ **************************

extern WiFiUDP udp;            // Объект транспорта сетевых пакетов

extern char   apName[];        // Имя сети в режиме точки доступа
extern char   apPass[];        // Пароль подключения к точке доступа
extern char   ssid[];          // SSID (имя) вашего роутера (конфигурируется подключением через точку доступа и сохранением в EEPROM)
extern char   pass[];          // пароль роутера

#define MEMFLAG 0
extern boolean   useDHCP;         // получать динамический IP
extern byte   IP_STA[];        // Статический адрес в локальной сети WiFi по умолчанию при первом запуске. Потом - загружается из настроек, сохраненных в EEPROM
extern byte   IP_STA[];        // Статический адрес в локальной сети WiFi по умолчанию при первом запуске. Потом - загружается из настроек, сохраненных в EEPROM
extern unsigned int localPort; // локальный порт на котором слушаются входящие команды управления от приложения на смартфоне, передаваемые через локальную сеть

// ------------------------ MQTT parameters --------------------

#if (USE_MQTT == 1)

extern  PubSubClient mqtt;     // Объект соединения с MQTT сервером

// Внимание!!! Если вы меняете эти значения ПОСЛЕ того, как прошивка уже хотя бы раз была загружена в плату и выполнялась,
// чтобы изменения вступили в силу нужно также изменить значение константы EEPROM_OK в строке 8 этого файла

#ifndef DEFAULT_MQTT_SERVER
#define DEFAULT_MQTT_SERVER "192.168.2.125"  // MQTT сервер
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER   "user"      // Имя mqtt-пользователя    (укажите имя пользователя для вашего соединения)
#endif

#ifndef DEFAULT_MQTT_PASS
#define DEFAULT_MQTT_PASS   "DINH6J6T"   // Пароль mqtt-пользователя (укажите пароль вашего соединения)
#endif

#ifndef DEFAULT_MQTT_PORT
#define DEFAULT_MQTT_PORT    1883                // Порт mqtt-соединения
#endif

#ifndef MQTT_SEND_DELAY                          // Отправлять сообщение на MQTT-сервер не чаще 1 сообщения в секунду (ограничение бесплатного MQTT сервера);
#define MQTT_SEND_DELAY      1                   // Сообщения, отправленные чаще защитного интервала "съедаются" сервером (игнорируются, пропадают); 
#endif                                           // Если нет ограничений на частоту отправки сообщений - поставьте здесь 0
                                                  

// Код работы с MQTT-каналом ориентирован на использование MQTT-брокера mqtt.4api.ru
// Для управления и отладки можно использовать одну из следующих консолей: client.mqtt.4api.ru, hivemq.com/demos/websocket-client

#define  TOPIC_CMD      "cmd"                    // Топик - получение команды управления от клиента
#define  TOPIC_DTA      "dta"                    // Топик - отправка запрошенных данных клиенту
#define  TOPIC_ERR      "err"                    // Топик - отправка уведомлений об ошибке клиенту
#define  TOPIC_STA      "sta"                    // Топик - отправка уведомления о (ре)старте микроконтроллера
#define  TOPIC_TME      "tme"                    // Топик - отправка клиенту сообщений о событиях времени
#define  TOPIC_PWR      "pwr"                    // Топик - отправка клиенту сообщений о включении/выключении устройства
#define  TOPIC_STT      "stt"                    // Топик - отправка клиенту сообщений о текущем статусе параметров устройства - основной набор параеметров (пакет)
#define  TOPIC_PROF     "prof"                   // Топик - отправка клиенту сообщений о профиле выращивания устройства
#define  TOPIC_CAL      "cal"                    // Топик - отправка клиенту сообщений при калибровке датчиков устройства
#define  TOPIC_REG      "reg"                    // Топик - отправка клиенту сообщений о совершеном воздействии устройства на стреду
#define  TOPIC_HWSET    "hwset"                  // Топик - отправка клиенту сообщений о сосостоянии аппаратных настроек
#define  TOPIC_HWSTAT   "hwstat"                 // Топик - отправка клиенту сообщений текущих показаниях
#define  TOPIC_MQTTSTT  "mqttstt"                // Топик - отправка клиенту сообщений о текущем статусе mqtt соединения

#ifdef HUMCONTROL
#define  TOPIC_TEMP     "temp"                   // Топик - отправка клиенту сообщений о текущей температуре
#define  TOPIC_HUM      "hum"                    // Топик - отправка клиенту сообщений о текущем 
#define  TOPIC_MAXHUM   "maxhum"                 // Топик - отправка клиенту значения максимальной влажности
#define  TOPIC_MINHUM   "minhum"                 // Топик - отправка клиенту значения минимальной влажности
#define  TOPIC_RELAY    "relay"                  // Топик - отправка клиенту сообщений статусе
#endif                                           // Если нет ограничений на частоту отправки сообщений - поставьте здесь 0

#ifdef PHTDSCONTROL

#define  TOPIC_tds      "tds"                  // MQTT Topic
#define  TOPIC_rawTDS   "rawTDS"               // MQTT Topic
#define  TOPIC_ph       "ph"                   // MQTT Topic
#define  TOPIC_rawPh    "rawPh"                // MQTT Topic
#define  TOPIC_Wtemp    "Wtemp"                // MQTT Topic

extern  float realPh, realTDS, Wtemp;

extern  boolean TDScal;    // TDS Calibration start 
extern  boolean PhCal;     //  Ph Calibration start
extern  boolean PhOk;      //  Ph Correction complete

extern  int rawPh, rawTDS;
extern  boolean RAWMode;   // RAW read mode

extern  float phmin, phmax, phk, PhMP, tdsk, TdsMP, PhCalP1, PhCalP2;

extern  uint16_t phVol, tdsAVol, tdsBVol, tdsCVol,
tdsmin, tdsmax, 
rawPhCalP1,  rawPhCalP2, 
rawTDSCalP1, rawTDSCalP2,
TDSCalP1, TDSCalP2,
phKa,  // усиление
phKb,  // средняя точка
tdsKa, // усиление
tdsKb; // средняя точка

#endif

extern  boolean     useMQTT;                  // Использовать канал управления через MQTT - флаг намерения    // При отключении из приложения set_useMQTT(false) устанавлифается соответствующее состояние (параметр QA), состояние 'намерение отключить MQTT'
extern  boolean     stopMQTT;                 // Использовать канал управления через MQTT - флаг результата   // которое должно быть отправлено на MQTT-сервер, значит реально состояние 'MQTT остановлен' - только после отправки флага QA на сервер
extern  char     mqtt_server[25] ;         // Имя сервера MQTT
extern  char     mqtt_user[15]   ;         // Логин от сервера
extern  char     mqtt_pass[15]   ;         // Пароль от сервера
extern  char     mqtt_prefix[31] ;         // Префикс топика сообщения
extern  uint16_t mqtt_port       ;         // Порт для подключения к серверу MQTT
extern  uint16_t mqtt_send_delay ;         // Задержка между последовательными обращениями к MQTT серверу
extern  boolean     mqtt_state_packet;        // Способ передачи состояния: true - в пакете, false - каждый параметр индивидуально

// Выделение места под массив команд, поступающих от MQTT-сервера
// Callback на поступление команды от MQTT сервера происходит асинхронно, и если предыдущая
// команда еще не обработалась - происходит новый вызов обработчика команд, который не рентабелен -
// это приводит к краху приложения. Чтобы избежать этого поступающие команды будем складывать в очередь 
// и выполнять их в основном цикле программы
#define  QSIZE_IN       8                  // размер очереди команд от MQTT
#define  QSIZE_OUT      96                 // размер очереди исходящих сообщений MQTT
extern String   cmdQueue[];                // Кольцевой буфер очереди полученных команд от MQTT
extern String   tpcQueue[];                // Кольцевой буфер очереди отправки команд в MQTT (topic)
extern String   outQueue[];                // Кольцевой буфер очереди отправки команд в MQTT (message)
extern boolean     rtnQueue[];                // Кольцевой буфер очереди отправки команд в MQTT (retain)
extern byte     queueWriteIdx;             // позиция записи в очередь обработки полученных команд
extern byte     queueReadIdx;              // позиция чтения из очереди обработки полученных команд
extern byte     queueLength;               // количество команд в очереди обработки полученных команд
extern byte     outQueueWriteIdx;          // позиция записи в очередь отправки MQTT сообщений
extern byte     outQueueReadIdx;           // позиция чтения из очереди отправки MQTT сообщений
extern byte     outQueueLength;            // количество команд в очереди отправки MQTT сообщений

extern String   last_mqtt_server;
extern uint16_t last_mqtt_port;

extern String   changed_keys;              // Строка, содержащая список измененных параметров, чье состояние требуется отправить серверу
extern boolean     mqtt_connecting;           // Выполняется подключение к MQTT (еще не установлено)
extern boolean     mqtt_topic_subscribed;     // Подписка на топик команд выполнена
extern byte     mqtt_conn_cnt;             // Счетчик попыток подключения для форматирования вывода
extern unsigned long mqtt_conn_last;       // Время последней попытки подключения к MQTT-серверу
extern unsigned long mqtt_send_last;       // Время последней отправки сообщения к MQTT-серверу
extern uint16_t upTimeSendInterval;        // Интервал отправки uptime в секундах, 0 если не нужно отправлять
extern unsigned long uptime_send_last;     // Время последней отправки uptime к MQTT-серверу по инициативе устройства
#endif

// --------------------Режимы работы Wifi соединения-----------------------

extern boolean   useSoftAP;            // использовать режим точки доступа
extern boolean   wifi_connected;       // true - подключение к wifi сети выполнена  
extern boolean   ap_connected;         // true - работаем в режиме точки доступа;

// **************** СИНХРОНИЗАЦИЯ ЧАСОВ ЧЕРЕЗ ИНТЕРНЕТ *******************

#define SYNCTIMEPERIOD  60          // Период синхронизации в минутах по умолчанию
#define NTP_PACKET_SIZE 48          // NTP время - в первых 48 байтах сообщения
#define TIMEZONE        7           // смещение часового пояса от UTC

extern IPAddress  timeServerIP;
extern uint16_t   syncTimePeriod;   // Период синхронизации в минутах по умолчанию
extern byte       packetBuffer[];   // буфер для хранения входящих и исходящих пакетов NTP

extern int8_t     timeZoneOffset;   // смещение часового пояса от UTC
extern long       ntp_t;            // Время, прошедшее с запроса данных с NTP-сервера (таймаут)
extern byte       ntp_cnt;          // Счетчик попыток получить данные от сервера
extern boolean       init_time;        // Флаг false - время не инициализировано; true - время инициализировано
extern boolean       refresh_time;     // Флаг true - пришло время выполнить синхронизацию часов с сервером NTP
extern boolean       useNtp;           // Использовать синхронизацию времени с NTP-сервером
extern boolean       getNtpInProgress; // Запрос времени с NTP сервера в процессе выполнения
extern char       ntpServerName[];  // Используемый сервер NTP

extern uint32_t   upTime ;          // время работы системы с последней перезагрузки

// ************************* ПРОЧИЕ ПЕРЕМЕННЫЕ *************************



// ---------------------------------------------------------------

// Сервер не может инициировать отправку сообщения клиенту - только в ответ на запрос клиента
// Следующие две переменные хранят сообщения, формируемые по инициативе сервера и отправляются в ответ на ближайший запрос от клиента,
// например в ответ на периодический ping - в команде sendAcknowledge();

extern String   cmd95;                        // Строка, формируемая sendPageParams(95) для отправки по инициативе сервера
extern String   cmd96;                        // Строка, формируемая sendPageParams(96) для отправки по инициативе сервера

// ---------------------------------------------------------------

extern int8_t   thisMode;                     // текущий режим - id
extern String   effect_name;                  // текущий режим - название

// ---------------------------------------------------------------
//extern timerMinim idleTimer(idleTime);             // Таймер бездействия ручного управления для автоперехода в демо-режим 

extern timerMinim saveSettingsTimer;          // Таймер отложенного сохранения настроек
extern timerMinim ntpSyncTimer;               // Сверяем время с NTP-сервером через syncTimePeriod минут
extern timerMinim AutoModeTimer;              // Таймер активации автоматического режима
extern timerMinim AutoFillTimer;              // Таймер активации автоматического долива
// ********************* ДЛЯ ПАРСЕРА КОМАНДНЫХ ПАКЕТОВ *************************

#define    BUF_MAX_SIZE  1024//4096               // максимальный размер выделяемого буфера для коммуникации по UDP каналу
#define    PARSE_AMOUNT  16                 // максимальное количество значений в массиве, который хотим получить
#define    header '$'                       // стартовый символ управляющей посылки
#define    divider ' '                      // разделительный символ
#define    ending ';'                       // завершающий символ
 
//extern int32_t    intData[];           // массив численных значений после парсинга - для WiFi часы время синхр м.б отрицательным + 
                                            // период синхронизации м.б больше 255 мин - нужен тип int32_t
extern char       incomeBuffer[];      // Буфер для приема строки команды из wifi udp сокета; также используется для загрузки строк из EEPROM
extern char       replyBuffer[];                  // ответ клиенту - подтверждения получения команды: "ack;/r/n/0"

//extern byte       ackCounter = 0;                  // счетчик отправляемых ответов для создания уникальности номера ответа

extern String     host_name;                       // Имя для регистрации в сети, а так же как имя клиента та сервере MQTT

// ------------------- ФАЙЛОВАЯ СИСТЕМА SPIFFS ----------------------

extern boolean       spiffs_ok;                    // Флаг - файловая система SPIFFS доступна для использования
extern size_t     spiffs_total_bytes;                   // Доступно байт в SPIFFS
extern size_t     spiffs_used_bytes;                    // Использовано байт в SPIFFS
extern int8_t     eeprom_backup;                    // Флаг - backup настроек 0 - нeт; 1 - FS; 2 - SD; 3 - FS и SD

#endif