#ifndef MQTT_H
#define MQTT_H
//#if (USE_MQTT == 1)

// Формирование топика сообщения
String mqtt_topic(String topic);
// Поместить сообщения для отправки на сервер в очередь
void putOutQueue(String topic, String message, boolean retain = false);
void SendMQTT(String &message, String topic);
void notifyUnknownCommand(const char* text);
boolean subscribeMqttTopics();
void checkMqttConnection();
// Отправка сообщений из очереди на червер
void processOutQueue();

#endif