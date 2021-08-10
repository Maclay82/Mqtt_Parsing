#ifndef DEF_SOFT_H
#define DEF_SOFT_H
#include <def_hard.h> // Определение параметров подключения и т.п


// Определения программных констант и переменных

extern unsigned long timing, timing1, per;

#ifdef HUMCONTROL
#ifndef MINMAXHUM
#define MINMAXHUM
#endif
extern float minhum, maxhum; // = minhumDEF // = maxhumDEF;
#endif

#ifdef PHTDSCONTROL
#define OPROSDELAY 150
#define NUM_AVER 20           // выборка (из скольки усредняем)
#endif

extern uint16_t AUTO_MODE_PERIOD;  // Период активации автоматического режима в минутах по умолчанию
extern bool     auto_mode;         // Флаг автоматического режима

// *************************** ПОДКЛЮЧЕНИЕ К СЕТИ **************************

extern WiFiUDP udp;            // Объект транспорта сетевых пакетов

extern char   apName[];        // Имя сети в режиме точки доступа
extern char   apPass[];        // Пароль подключения к точке доступа
extern char   ssid[];          // SSID (имя) вашего роутера (конфигурируется подключением через точку доступа и сохранением в EEPROM)
extern char   pass[];          // пароль роутера
extern byte   IP_STA[];        // Статический адрес в локальной сети WiFi по умолчанию при первом запуске. Потом - загружается из настроек, сохраненных в EEPROM
extern unsigned int localPort; // локальный порт на котором слушаются входящие команды управления от приложения на смартфоне, передаваемые через локальную сеть

// ------------------------ MQTT parameters --------------------

#if (USE_MQTT == 1)

extern  PubSubClient mqtt;     // Объект соединения с MQTT сервером

// Внимание!!! Если вы меняете эти значения ПОСЛЕ того, как прошивка уже хотя бы раз была загружена в плату и выполнялась,
// чтобы изменения вступили в силу нужно также изменить значение константы EEPROM_OK в строке 8 этого файла

#ifndef DEFAULT_MQTT_SERVER
#define DEFAULT_MQTT_SERVER "192.168.1.166" // MQTT сервер
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER   "user"      // Имя mqtt-пользователя    (укажите имя пользователя для вашего соединения)
#endif

#ifndef DEFAULT_MQTT_PASS
#define DEFAULT_MQTT_PASS   "DINH6J6T"      // Пароль mqtt-пользователя (укажите пароль вашего соединения)
#endif

#ifndef DEFAULT_MQTT_PORT
#define DEFAULT_MQTT_PORT    1883                // Порт mqtt-соединения
#endif

#ifndef MQTT_SEND_DELAY                          // Отправлять сообщение на MQTT-сервер не чаще 1 сообщения в секунду (ограничение бесплатного MQTT сервера);
#define MQTT_SEND_DELAY     0                    // Сообщения, отправленные чаще защитного интервала "съедаются" сервером (игнорируются, пропадают); 
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

#define  TOPIC_phKa     "phKa"                 // MQTT Topic
#define  TOPIC_phKb     "phKb"                 // MQTT Topic
#define  TOPIC_tdsKa    "tdsKa"                // MQTT Topic
#define  TOPIC_tdsKb    "tdsKb"                // MQTT Topic
#define  TOPIC_phCP1    "phCP1"                // MQTT Topic
#define  TOPIC_phCP2    "phCP2"                // MQTT Topic
#define  TOPIC_tdsCP1   "tdsCP1"               // MQTT Topic
#define  TOPIC_tdsCP2   "tdsCP2"               // MQTT Topic

extern  float realPh, realTDS,
Wtemp;

extern  boolean TDScalib;  // TDS Calibration complete 
extern  boolean Phcalib;  //  Ph Calibration complete

extern  int rawPh, rawTDS;
extern  boolean RAWMode;  // RAW read mode

extern  float phmin, phmax, tdsmin, tdsmax, 
              phk, phb, tdsk, tdsb, PhCalp1, PhCalp2, TDSCalp1, TDSCalp2;

extern  int rawPhCalp1, rawPhCalp2, rawTDSCalp1, rawTDSCalp2,
phKa,  // усиление
phKb,  // средняя точка
tdsKa, // усиление
tdsKb; // средняя точка

#endif

extern  bool     useMQTT;                        // Использовать канал управления через MQTT - флаг намерения    // При отключении из приложения set_useMQTT(false) устанавлифается соответствующее состояние (параметр QA), состояние 'намерение отключить MQTT'
extern  bool     stopMQTT;                       // Использовать канал управления через MQTT - флаг результата   // которое должно быть отправлено на MQTT-сервер, значит реально состояние 'MQTT остановлен' - только после отправки флага QA на сервер
extern  char     mqtt_server[25] ;               // Имя сервера MQTT
extern  char     mqtt_user[15]   ;               // Логин от сервера
extern  char     mqtt_pass[15]   ;               // Пароль от сервера
extern  char     mqtt_prefix[31] ;               // Префикс топика сообщения
extern  uint16_t mqtt_port       ;               // Порт для подключения к серверу MQTT
extern  uint16_t mqtt_send_delay ;               // Задержка между последовательными обращениями к MQTT серверу
extern  bool     mqtt_state_packet;              // Способ передачи состояния: true - в пакете, false - каждый параметр индивидуально

// Выделение места под массив команд, поступающих от MQTT-сервера
// Callback на поступление команды от MQTT сервера происходит асинхронно, и если предыдущая
// команда еще не обработалась - происходит новый вызов обработчика команд, который не рентабелен -
// это приводит к краху приложения. Чтобы избежать этого поступающие команды будем складывать в очередь 
// и выполнять их в основном цикле программы
#define  QSIZE_IN 8                         // размер очереди команд от MQTT
#define  QSIZE_OUT 96                       // размер очереди исходящих сообщений MQTT
extern String   cmdQueue[];                // Кольцевой буфер очереди полученных команд от MQTT
extern String   tpcQueue[];               // Кольцевой буфер очереди отправки команд в MQTT (topic)
extern String   outQueue[];               // Кольцевой буфер очереди отправки команд в MQTT (message)
extern bool     rtnQueue[];               // Кольцевой буфер очереди отправки команд в MQTT (retain)
extern byte     queueWriteIdx;                 // позиция записи в очередь обработки полученных команд
extern byte     queueReadIdx;                  // позиция чтения из очереди обработки полученных команд
extern byte     queueLength;                   // количество команд в очереди обработки полученных команд
extern byte     outQueueWriteIdx;              // позиция записи в очередь отправки MQTT сообщений
extern byte     outQueueReadIdx;               // позиция чтения из очереди отправки MQTT сообщений
extern byte     outQueueLength;                // количество команд в очереди отправки MQTT сообщений

extern String   last_mqtt_server;
extern uint16_t last_mqtt_port;

extern String   changed_keys;                 // Строка, содержащая список измененных параметров, чье состояние требуется отправить серверу
extern bool     mqtt_connecting;           // Выполняется подключение к MQTT (еще не установлено)
extern bool     mqtt_topic_subscribed;     // Подписка на топик команд выполнена
extern byte     mqtt_conn_cnt;                 // Счетчик попыток подключения для форматирования вывода
extern unsigned long mqtt_conn_last;               // Время последней попытки подключения к MQTT-серверу
extern unsigned long mqtt_send_last;               // Время последней отправки сообщения к MQTT-серверу
extern uint16_t upTimeSendInterval;            // Интервал отправки uptime в секундах, 0 если не нужно отправлять
extern unsigned long uptime_send_last;             // Время последней отправки uptime к MQTT-серверу по инициативе устройства
#endif

// --------------------Режимы работы Wifi соединения-----------------------

extern bool   useSoftAP;                   // использовать режим точки доступа
extern bool   wifi_connected;              // true - подключение к wifi сети выполнена  
extern bool   ap_connected;                // true - работаем в режиме точки доступа;

// **************** СИНХРОНИЗАЦИЯ ЧАСОВ ЧЕРЕЗ ИНТЕРНЕТ *******************

extern IPAddress timeServerIP;
#define NTP_PACKET_SIZE 48                  // NTP время - в первых 48 байтах сообщения
extern uint16_t SYNC_TIME_PERIOD;             // Период синхронизации в минутах по умолчанию
extern byte packetBuffer[];         // буфер для хранения входящих и исходящих пакетов NTP

extern int8_t timeZoneOffset;       // смещение часового пояса от UTC
extern long   ntp_t;                // Время, прошедшее с запроса данных с NTP-сервера (таймаут)
extern byte   ntp_cnt;              // Счетчик попыток получить данные от сервера
extern bool   init_time;            // Флаг false - время не инициализировано; true - время инициализировано
extern bool   refresh_time;         // Флаг true - пришло время выполнить синхронизацию часов с сервером NTP
extern bool   useNtp;               // Использовать синхронизацию времени с NTP-сервером
extern bool   getNtpInProgress;     // Запрос времени с NTP сервера в процессе выполнения
extern char   ntpServerName[];      // Используемый сервер NTP

extern uint32_t upTime ;            // время работы системы с последней перезагрузки

// ************************* ПРОЧИЕ ПЕРЕМЕННЫЕ *************************



// ---------------------------------------------------------------

// Сервер не может инициировать отправку сообщения клиенту - только в ответ на запрос клиента
// Следующие две переменные хранят сообщения, формируемые по инициативе сервера и отправляются в ответ на ближайший запрос от клиента,
// например в ответ на периодический ping - в команде sendAcknowledge();

extern String   cmd95;                        // Строка, формируемая sendPageParams(95) для отправки по инициативе сервера
extern String   cmd96;                        // Строка, формируемая sendPageParams(96) для отправки по инициативе сервера

// ---------------------------------------------------------------

extern int8_t   thisMode;                      // текущий режим - id
extern String   effect_name;                  // текущий режим - название

// ---------------------------------------------------------------
//extern timerMinim idleTimer(idleTime);             // Таймер бездействия ручного управления для автоперехода в демо-режим 

extern timerMinim saveSettingsTimer;        // Таймер отложенного сохранения настроек
extern timerMinim ntpSyncTimer;             // Сверяем время с NTP-сервером через SYNC_TIME_PERIOD минут
extern timerMinim AutoModeTimer;             // Таймер активации автоматического режима
// ********************* ДЛЯ ПАРСЕРА КОМАНДНЫХ ПАКЕТОВ *************************

#define    BUF_MAX_SIZE  4096               // максимальный размер выделяемого буфера для коммуникации по UDP каналу
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

extern bool       spiffs_ok;                    // Флаг - файловая система SPIFFS доступна для использования
extern size_t     spiffs_total_bytes;                   // Доступно байт в SPIFFS
extern size_t     spiffs_used_bytes;                    // Использовано байт в SPIFFS
extern int8_t     eeprom_backup;                    // Флаг - backup настроек 0 - нeт; 1 - FS; 2 - SD; 3 - FS и SD

#endif