#ifndef MQTT_H
#define MQTT_H
//#if (USE_MQTT == 1)

// Набор параметров, который требуется отправлять MQTT-клиенту на изменение их состояния
// STATE_KEYS начинается с '|' и заканчивается на '|' для удобства поиска / проверки наличия ключа в строке,
// которые должны быть удалены перед использованием далее для перебора ключей
// Если вам не нужен на стороне MQTT клиента полный перечент параметров - оставьте только те, что вам нужны
#define STATE_KEYS "|W|H|DM|PS|PD|IT|AL|RM|PW|BR|WU|WT|WR|WS|WC|WN|WZ|EF|EN|UE|UT|UC|SE|SS|SQ|BE|CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF|TM|AW|AT|AD|AE|MX|MU|MD|MV|MA|MB|MP|AU|AN|NW|IP|QZ|QA|QP|QS|QU|QD|QR|TE|TI|TS|CT|C2|OM|ST|AM1T|AM1A|AM2T|AM2A|AM3T|AM3A|AM4T|AM4A|AM5A|AM6A|UI|UP|"

// Формирование топика сообщения
String mqtt_topic(String topic);
// Поместить сообщения для отправки на сервер в очередь
void putOutQueue(String topic, String message, bool retain = false);
void SendMQTT(String &message, String topic);
void notifyUnknownCommand(const char* text);
bool subscribeMqttTopicCmd();
void checkMqttConnection();
// Отправка в MQTT канал - текущие значения переменных
void SendCurrentState(String keys, String topic, bool immediate);
// Получение строки пары ключ-значение в формате json;
String getKVP(String &key, JsonVariant &value);
// Отправка в MQTT канал  состояния всех параметров при старте прошивки
void mqttSendStartState();
// Отправка сообщений из очереди на червер
void processOutQueue();

#endif