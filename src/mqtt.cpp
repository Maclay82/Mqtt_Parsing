#include <Arduino.h>
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

WiFiClient m_client;                            // Объект для работы с удалёнными хостами - соединение с MQTT-сервером
PubSubClient mqtt(m_client);                    // Объект соединения с MQTT сервером


boolean     useMQTT = true;                         // Использовать канал управления через MQTT - флаг намерения    // При отключении из приложения set_useMQTT(false) устанавлифается соответствующее состояние (параметр QA), состояние 'намерение отключить MQTT'
boolean     stopMQTT = false;                       // Использовать канал управления через MQTT - флаг результата   // которое должно быть отправлено на MQTT-сервер, значит реально состояние 'MQTT остановлен' - только после отправки флага QA на сервер
char     mqtt_server[25] = DEFAULT_MQTT_SERVER;  // Имя сервера MQTT
char     mqtt_user[15]   = "";                   // Логин от сервера
char     mqtt_pass[15]   = "";                   // Пароль от сервера
char     mqtt_prefix[31] = "";                   // Префикс топика сообщения
uint16_t mqtt_port       = DEFAULT_MQTT_PORT;    // Порт для подключения к серверу MQTT
uint16_t mqtt_send_delay = MQTT_SEND_DELAY;      // Задержка между последовательными обращениями к MQTT серверу
boolean     mqtt_state_packet = true;               // Способ передачи состояния: true - в пакете, false - каждый параметр индивидуально

String   cmdQueue[QSIZE_IN];                // Кольцевой буфер очереди полученных команд от MQTT
String   tpcQueue[QSIZE_OUT];               // Кольцевой буфер очереди отправки команд в MQTT (topic)
String   outQueue[QSIZE_OUT];               // Кольцевой буфер очереди отправки команд в MQTT (message)
boolean     rtnQueue[QSIZE_OUT];               // Кольцевой буфер очереди отправки команд в MQTT (retain)

byte     queueWriteIdx = 0;                 // позиция записи в очередь обработки полученных команд
byte     queueReadIdx = 0;                  // позиция чтения из очереди обработки полученных команд
byte     queueLength = 0;                   // количество команд в очереди обработки полученных команд
byte     outQueueWriteIdx = 0;              // позиция записи в очередь отправки MQTT сообщений
byte     outQueueReadIdx = 0;               // позиция чтения из очереди отправки MQTT сообщений
byte     outQueueLength = 0;                // количество команд в очереди отправки MQTT сообщений

String   last_mqtt_server = "";
uint16_t last_mqtt_port = 0;

String   changed_keys = "";                 // Строка, содержащая список измененных параметров, чье состояние требуется отправить серверу
boolean     mqtt_connecting = false;           // Выполняется подключение к MQTT (еще не установлено)
boolean     mqtt_topic_subscribed = false;     // Подписка на топик команд выполнена
byte     mqtt_conn_cnt = 0;                 // Счетчик попыток подключения для форматирования вывода
unsigned long mqtt_conn_last = 0;           // Время последней попытки подключения к MQTT-серверу
unsigned long mqtt_send_last = 0;           // Время последней отправки сообщения к MQTT-серверу
uint16_t upTimeSendInterval = 0;            // Интервал отправки uptime в секундах, 0 если не нужно отправлять
unsigned long uptime_send_last;             // Время последней отправки uptime к MQTT-серверу по инициативе устройства

#if (USE_MQTT == 1)

// Формирование топика сообщения
String mqtt_topic(String topic) {
  String ret_topic = String('/') + String(mqtt_prefix);
  if (ret_topic.length() > 0 && !ret_topic.endsWith("/")) ret_topic += "/";
  return ret_topic + String(HOST_NAME) + "/" + topic;
}

// Поместить сообщения для отправки на сервер в очередь
void putOutQueue(String topic, String message, boolean retain) {
  if (stopMQTT) return;
  boolean ok = false;
  // Если в настройках сервера MQTT нет задержки между отправками сообщений - пытаемся отправить сразу без помещения в очередь
  if (mqtt_send_delay == 0) {
    ok = mqtt.beginPublish(topic.c_str(), message.length(), retain);
    if (ok) {
      // Если инициация отправки была успешной - заполняем буфер отправки передаваемой строкой сообщения
      mqtt.print(message.c_str());
      // Завершаем отправку. Если пакет был отправлен - возвращается 1, если ошибка отправки - возвращается 0
      ok = mqtt.endPublish() == 1;
      if (ok) {
        // Отправка прошла успешно
        Serial.print(F("MQTT >> OK >> ")); 
        Serial.print(topic);
        Serial.print(F(" >> \t")); 
        Serial.println(message);
      }
    }
  }
  // Если отправка не произошла и в очереди есть место - помещаем сообщение в очередь отправки
  if (!ok && outQueueLength < QSIZE_OUT) {
    outQueueLength++;
    tpcQueue[outQueueWriteIdx] = topic;
    outQueue[outQueueWriteIdx] = message;
    rtnQueue[outQueueWriteIdx] = retain;
    outQueueWriteIdx++;
    if (outQueueWriteIdx >= QSIZE_OUT) outQueueWriteIdx = 0;
  }
}

void SendMQTT(String &message, String topic) {
  if (stopMQTT) return;
  putOutQueue(mqtt_topic(topic), message);
}

void notifyUnknownCommand(const char* text) {
  if (stopMQTT) return;
  DynamicJsonDocument doc(256);
  String out;
  doc["message"] = F("unknown command");
  doc["text"]    = String(F("неизвестная команда '")) + String(text) + String("'");
  serializeJson(doc, out);      
  SendMQTT(out, TOPIC_ERR);
}

boolean subscribeMqttTopics() {
  boolean ok = false;
  if (mqtt.connected() && millis() - mqtt_send_last > mqtt_send_delay) {
    Serial.print(F("Подписка на topic='cmd' >> "));
    Serial.print(mqtt_topic(TOPIC_CMD));
    Serial.print('\t');
    ok = mqtt.subscribe(mqtt_topic(TOPIC_CMD).c_str());
    if (ok) Serial.println(F("OK"));
    else    Serial.println(F("FAIL"));

#ifdef HUMCONTROL //or CO2CONTROL
    Serial.print(F("Подписка на topic='relay' >> "));
    Serial.print(mqtt_topic(TOPIC_RELAY));
    Serial.print('\t');
    ok = mqtt.subscribe(mqtt_topic(TOPIC_RELAY).c_str());
    if (ok) Serial.println(F("OK"));
    else    Serial.println(F("FAIL"));
#endif

#ifdef PHTDSCONTROL
#endif

    mqtt_send_last = millis();
  }
  return ok;
}

void checkMqttConnection() {

  // Ели нет оединения  интернетом - незачем проверять наличие подключения к MQTT-ерверу
  if (!wifi_connected) {
    return;
  }
  // Проверить - выполнена ли подписка на топик команд, если нет - подписаться
  if (!stopMQTT && !mqtt_topic_subscribed) {
    mqtt_topic_subscribed = subscribeMqttTopics();
  }
  // Отправить сообщение из очереди, если очередь содержит сообщения
  // Проверка наличия подключения и защитны интервала отправки проверяется внутри вызова
  if (!stopMQTT && mqtt_topic_subscribed) {
    processOutQueue();
  }
  // Если связь с MQTT сервером не установлена - выполнить переподключение к серверу
  // Слишком частая проверка мешает приложению на смартфоне общаться с программой - запрос блокирующий и при неответе MQTT сервера
  // слишком частые попытки подключения к MQTT серверу не дают передаваться данным из / в приложение - приложение "отваливается"
  if (!stopMQTT && !mqtt.connected() && (mqtt_conn_last == 0 || (millis() - mqtt_conn_last > 3500))) 
  {
    String clientId = String(host_name);
    clientId += String("-");
    clientId += String(random(0xffff), HEX);
    if (!mqtt_connecting) {
      Serial.print(F("\nПодключаемся к MQTT-серверу '"));
      Serial.print(mqtt_server);
      Serial.print(":");
      Serial.print(mqtt_port);
      Serial.print(F("'; ClientID -> '"));
      Serial.print(clientId);
      Serial.print(F("' ..."));
    }
    mqtt_topic_subscribed = false;
    mqtt_conn_last = millis();

    String topic = mqtt_topic(TOPIC_MQTTSTT);

    if (mqtt.connect(clientId.c_str(), mqtt_user, mqtt_pass, topic.c_str(), 0, true, "offline")) 
    {
      Serial.print(F("\nПодключение к MQTT-серверу выполнено."));
      if (outQueueLength > 0) {
        Serial.print(F("Сообщений в очереди отправки: "));  
        Serial.println(outQueueLength);  
      }
      putOutQueue(topic, "online", true);
      mqtt_connecting = false;      
    } 
    else 
    {      
      Serial.print(".");
      mqtt_connecting = true;
      mqtt_conn_cnt++;
      if (mqtt_conn_cnt == 80) {
        mqtt_conn_cnt = 0;
        Serial.println();
      }
    }
  }
}

// Отправка сообщений из очереди на сервер
void processOutQueue() {
  if (stopMQTT) {
    outQueueReadIdx = 0;
    outQueueWriteIdx = 0;
    outQueueLength = 0;
    return;
  }

  if (mqtt.connected() && outQueueLength > 0 && millis() - mqtt_send_last >= mqtt_send_delay) {    
    // Топик и содержимое отправляемого сообщения
    String topic = tpcQueue[outQueueReadIdx];
    String message = outQueue[outQueueReadIdx];
    boolean   retain = rtnQueue[outQueueReadIdx];
    // Пытаемся отправить. Если инициализация отправки не удалась - возвращается false; Если удалась - true
      // Serial.print(F("Пытаемся отправить.")); 
      // Serial.print(F("topic - ")); 
      // Serial.print(topic); 
      // Serial.print(F(" >> ")); 
      // Serial.print(message);
    boolean ok = mqtt.beginPublish(topic.c_str(), message.length(), retain);
    if (ok) {
//      Serial.print(F("успешно")); 
      // Если инициация отправки была успешной - заполняем буфер отправки передаваемой строкой сообщения
      mqtt.print(message.c_str());
      // Завершаем отправку. Если пакет был отправлен - возвращается 1, если ошибка отправки - возвращается 0
      ok = mqtt.endPublish() == 1;
    }
    if (ok) {      
      // Отправка прошла успешно
      Serial.print(F("MQTT >> OK >> ")); 
      Serial.print(topic);
      Serial.print(F(" >> \t")); 
      Serial.println(message);
      // Извлекаем сообщение из очереди
      tpcQueue[outQueueReadIdx] = "";
      outQueue[outQueueReadIdx] = "";
      rtnQueue[outQueueReadIdx] = false;
      outQueueReadIdx++;
      if (outQueueReadIdx >= QSIZE_OUT) outQueueReadIdx = 0;
      outQueueLength--;
    } else {
      // Отправка не удалась
      Serial.print(F("MQTT >> FAIL >> ")); 
      Serial.print(topic);
      Serial.print(F(" >> \t")); 
      Serial.println(message);
    }
    // Запоминаем время отправки. Бесплатный сервер не позволяет отправлять сообщения чаще чем одно сообщение в секунду
    mqtt_send_last = millis();
  }  
}

#endif
