#ifndef DEF_SOFT_H
#define DEF_SOFT_H
#include <def_hard.h> // Определение параметров матрицы, пинов подключения и т.п


// Определения программных констант и переменных

#ifndef MINMAXHUM
#define MINMAXHUM
extern float minhum, maxhum; // = minhumDEF // = maxhumDEF;
#endif

// *************************** ПОДКЛЮЧЕНИЕ К СЕТИ **************************

static WiFiUDP udp;                                // Объект транспорта сетевых пакетов
                                            // к длине +1 байт на \0 - терминальный символ. Это буферы для загрузки имен/пароля из EEPROM. Значения задаются в define выше
static char   apName[11] = DEFAULT_AP_NAME;        // Имя сети в режиме точки доступа
static char   apPass[17] = DEFAULT_AP_PASS;        // Пароль подключения к точке доступа
static char   ssid[25]   = NETWORK_SSID;           // SSID (имя) вашего роутера (конфигурируется подключением через точку доступа и сохранением в EEPROM)
static char   pass[17]   = NETWORK_PASS;           // пароль роутера
static byte   IP_STA[]   = DEFAULT_IP;             // Статический адрес в локальной сети WiFi по умолчанию при первом запуске. Потом - загружается из настроек, сохраненных в EEPROM
static unsigned int localPort = 2390;              // локальный порт на котором слушаются входящие команды управления от приложения на смартфоне, передаваемые через локальную сеть

// ------------------------ MQTT parameters --------------------

#if (USE_MQTT == 1)

static WiFiClient m_client;                            // Объект для работы с удалёнными хостами - соединение с MQTT-сервером
static PubSubClient mqtt(m_client);                    // Объект соединения с MQTT сервером
//PubSubClient client(m_client);

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


#if (DEVICE_ID == 0)
#ifndef DEFAULT_MQTT_PREFIX
#define DEFAULT_MQTT_PREFIX "ghTest"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#endif
#endif

#if (DEVICE_ID == 1)
#ifndef DEFAULT_MQTT_PREFIX
#define DEFAULT_MQTT_PREFIX "gh1"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#endif
#endif

#if (DEVICE_ID == 2)
#ifndef DEFAULT_MQTT_PREFIX
#define DEFAULT_MQTT_PREFIX "gh2"      // Префикс топика сообщения или пустая строка, если префикс не требуется
#endif
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

static bool     useMQTT = true;                         // Использовать канал управления через MQTT - флаг намерения    // При отключении из приложения set_useMQTT(false) устанавлифается соответствующее состояние (параметр QA), состояние 'намерение отключить MQTT'
static bool     stopMQTT = false;                       // Использовать канал управления через MQTT - флаг результата   // которое должно быть отправлено на MQTT-сервер, значит реально состояние 'MQTT остановлен' - только после отправки флага QA на сервер
static char     mqtt_server[25] = DEFAULT_MQTT_SERVER;                   // Имя сервера MQTT
static char     mqtt_user[15]   = DEFAULT_MQTT_USER;//"";                   // Логин от сервера
static char     mqtt_pass[15]   = DEFAULT_MQTT_PASS;//"";                   // Пароль от сервера
static char     mqtt_prefix[31] = DEFAULT_MQTT_PREFIX;//"";                   // Префикс топика сообщения
static uint16_t mqtt_port       = DEFAULT_MQTT_PORT;    // Порт для подключения к серверу MQTT
static uint16_t mqtt_send_delay = MQTT_SEND_DELAY;      // Задержка между последовательными обращениями к MQTT серверу
static bool     mqtt_state_packet = true;               // Способ передачи состояния: true - в пакете, false - каждый параметр индивидуально

// Выделение места под массив команд, поступающих от MQTT-сервера
// Callback на поступление команды от MQTT сервера происходит асинхронно, и если предыдущая
// команда еще не обработалась - происходит новый вызов обработчика команд, который не рентабелен -
// это приводит к краху приложения. Чтобы избежать этого поступающие команды будем складывать в очередь 
// и выполнять их в основном цикле программы
#define  QSIZE_IN 8                         // размер очереди команд от MQTT
#define  QSIZE_OUT 96                       // размер очереди исходящих сообщений MQTT
static String   cmdQueue[QSIZE_IN];                // Кольцевой буфер очереди полученных команд от MQTT
static String   tpcQueue[QSIZE_OUT];               // Кольцевой буфер очереди отправки команд в MQTT (topic)
static String   outQueue[QSIZE_OUT];               // Кольцевой буфер очереди отправки команд в MQTT (message)
static bool     rtnQueue[QSIZE_OUT];               // Кольцевой буфер очереди отправки команд в MQTT (retain)
static byte     queueWriteIdx = 0;                 // позиция записи в очередь обработки полученных команд
static byte     queueReadIdx = 0;                  // позиция чтения из очереди обработки полученных команд
static byte     queueLength = 0;                   // количество команд в очереди обработки полученных команд
static byte     outQueueWriteIdx = 0;              // позиция записи в очередь отправки MQTT сообщений
static byte     outQueueReadIdx = 0;               // позиция чтения из очереди отправки MQTT сообщений
static byte     outQueueLength = 0;                // количество команд в очереди отправки MQTT сообщений

static String   last_mqtt_server = "";
static uint16_t last_mqtt_port = 0;

static String   changed_keys = "";                 // Строка, содержащая список измененных параметров, чье состояние требуется отправить серверу
static bool     mqtt_connecting = false;           // Выполняется подключение к MQTT (еще не установлено)
static bool     mqtt_topic_subscribed = false;     // Подписка на топик команд выполнена
static byte     mqtt_conn_cnt = 0;                 // Счетчик попыток подключения для форматирования вывода
static unsigned long mqtt_conn_last;               // Время последней попытки подключения к MQTT-серверу
static unsigned long mqtt_send_last;               // Время последней отправки сообщения к MQTT-серверу
static uint16_t upTimeSendInterval = 0;            // Интервал отправки uptime в секундах, 0 если не нужно отправлять
static unsigned long uptime_send_last;             // Время последней отправки uptime к MQTT-серверу по инициативе устройства
#endif

// --------------------Режимы работы Wifi соединения-----------------------

static bool   useSoftAP = false;                   // использовать режим точки доступа
static bool   wifi_connected = false;              // true - подключение к wifi сети выполнена  
static bool   ap_connected = false;                // true - работаем в режиме точки доступа;

// **************** СИНХРОНИЗАЦИЯ ЧАСОВ ЧЕРЕЗ ИНТЕРНЕТ *******************

static IPAddress timeServerIP;
#define NTP_PACKET_SIZE 48                  // NTP время - в первых 48 байтах сообщения
static uint16_t SYNC_TIME_PERIOD = 60;             // Период синхронизации в минутах по умолчанию
static byte packetBuffer[NTP_PACKET_SIZE];         // буфер для хранения входящих и исходящих пакетов NTP

static int8_t timeZoneOffset = 7;                  // смещение часового пояса от UTC
static long   ntp_t = 0;                           // Время, прошедшее с запроса данных с NTP-сервера (таймаут)
static byte   ntp_cnt = 0;                         // Счетчик попыток получить данные от сервера
static bool   init_time = false;                   // Флаг false - время не инициализировано; true - время инициализировано
static bool   refresh_time = true;                 // Флаг true - пришло время выполнить синхронизацию часов с сервером NTP
static bool   useNtp = true;                       // Использовать синхронизацию времени с NTP-сервером
static bool   getNtpInProgress = false;            // Запрос времени с NTP сервера в процессе выполнения
static char   ntpServerName[31] = "";              // Используемый сервер NTP

static uint32_t upTime = 0;                        // время работы системы с последней перезагрузки

// ************************* ПРОЧИЕ ПЕРЕМЕННЫЕ *************************

static bool     eepromModified = false;            // флаг: EEPROM изменен, требует сохранения

// ---------------------------------------------------------------

// Сервер не может инициировать отправку сообщения клиенту - только в ответ на запрос клиента
// Следующие две переменные хранят сообщения, формируемые по инициативе сервера и отправляются в ответ на ближайший запрос от клиента,
// например в ответ на периодический ping - в команде sendAcknowledge();

static String   cmd95 = "";                        // Строка, формируемая sendPageParams(95) для отправки по инициативе сервера
static String   cmd96 = "";                        // Строка, формируемая sendPageParams(96) для отправки по инициативе сервера

// ---------------------------------------------------------------

static int8_t   thisMode = 0;                      // текущий режим - id
static String   effect_name = "";                  // текущий режим - название

// ---------------------------------------------------------------
//timerMinim idleTimer(idleTime);             // Таймер бездействия ручного управления для автоперехода в демо-режим 

static timerMinim saveSettingsTimer(15000);        // Таймер отложенного сохранения настроек
static timerMinim ntpSyncTimer(1000 * 60 * SYNC_TIME_PERIOD);    // Сверяем время с NTP-сервером через SYNC_TIME_PERIOD минут

// ********************* ДЛЯ ПАРСЕРА КОМАНДНЫХ ПАКЕТОВ *************************

#define    BUF_MAX_SIZE  4096               // максимальный размер выделяемого буфера для коммуникации по UDP каналу
#define    PARSE_AMOUNT  16                 // максимальное количество значений в массиве, который хотим получить
#define    header '$'                       // стартовый символ управляющей посылки
#define    divider ' '                      // разделительный символ
#define    ending ';'                       // завершающий символ
 
static int32_t    intData[PARSE_AMOUNT];           // массив численных значений после парсинга - для WiFi часы время синхр м.б отрицательным + 
                                            // период синхронизации м.б больше 255 мин - нужен тип int32_t
static char       incomeBuffer[BUF_MAX_SIZE];      // Буфер для приема строки команды из wifi udp сокета; также используется для загрузки строк из EEPROM
static char       replyBuffer[8];                  // ответ клиенту - подтверждения получения команды: "ack;/r/n/0"

static byte       ackCounter = 0;                  // счетчик отправляемых ответов для создания уникальности номера ответа

#if (USE_MQTT == 1)
#define    BUF_MQTT_SIZE  384               // максимальный размер выделяемого буфера для входящих сообщений по MQTT каналу
static char       incomeMqttBuffer[BUF_MQTT_SIZE]; // Буфер для приема строки команды из MQTT
#endif

// --------------- ВРЕМЕННЫЕ ПЕРЕМЕННЫЕ ПАРСЕРА ------------------

static boolean    recievedFlag;                               // буфер содержит принятые данные
static boolean    parseStarted;
static byte       parse_index;
static String     string_convert = "";
static String     receiveText = "";
static bool       haveIncomeData = false;
static char       incomingByte;

static int16_t    bufIdx = 0;                                 // Могут приниматься пакеты > 255 байт - тип int16_t
static int16_t    packetSize = 0;

static String     host_name = "";                       // Имя для регистрации в сети, а так же как имя клиента та сервере MQTT

// ------------------- ФАЙЛОВАЯ СИСТЕМА SPIFFS ----------------------

static bool       spiffs_ok = false;                    // Флаг - файловая система SPIFFS доступна для использования
static size_t     spiffs_total_bytes;                   // Доступно байт в SPIFFS
static size_t     spiffs_used_bytes;                    // Использовано байт в SPIFFS
static int8_t     eeprom_backup = 0;                    // Флаг - backup настроек 0 - нeт; 1 - FS; 2 - SD; 3 - FS и SD

#endif