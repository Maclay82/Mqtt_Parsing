#ifndef SETTERS_H
#define SETTERS_H
//#include <Arduino.h>
//#include "def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
//#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

// Для поддержки работы MQTT нужно знать какой параметр изменился, чтобы отправить изменения на сервер
// По этой причмне нельзя напрямую присваивать новое значение переменной, нужно выполнить дополнительный действия,
// чтобы зафиксировать изменение значения. Для этой цели данная страница содержит функции - сеттеры, 
// устанавливающие значения переменных.

// Добавление ключа (параметра) в список изменившихся параметров, чьи новые значения необходимо отправить на сервер
void addKeyToChanged(String key);

// NP useNtp
void set_useNtp(bool value);
// NT SYNC_TIME_PERIOD
void set_SYNC_TIME_PERIOD(uint16_t value);
// NZ timeZoneOffset
void set_timeZoneOffset(int16_t value);
// NS ntpServerName
void set_ntpServerName(String value);
// NW ssid
void set_Ssid(String value);
// NA pass
void set_pass(String value);
// AN apName
void set_SoftAPName(String value);

// AA apPass
void set_SoftAPPass(String value);
// IP wifi_connected
void set_wifi_connected(bool value);
// IP IP_STA[]
void set_StaticIP(byte p1, byte p2, byte p3, byte p4);
// AU useSoftAP
void set_useSoftAP(bool value);

#if (USE_MQTT == 1)
// QA useMQTT
void set_useMQTT(bool value);

// QP mqtt_port
void set_mqtt_port(int16_t value);
// QS mqtt_server
void set_MqttServer(String value);
// QU mqtt_user
void set_MqttUser(String value);
// QW mqtt_pass
void set_MqttPass(String value);
// QD mqtt_send_delay
void set_mqtt_send_delay(int16_t value);
// QR mqtt_prefix
void set_MqttPrefix(String value);
// QK mqtt_state_packet
void set_mqtt_state_packet(bool value);
// UI upTimeSendInterval
void set_upTimeSendInterval(uint16_t value);
#endif
#endif
