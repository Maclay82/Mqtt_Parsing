//#include <Arduino.h>
//#include "a_def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
//#include "a_def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

// Для поддержки работы MQTT нужно знать какой параметр изменился, чтобы отправить изменения на сервер
// По этой причмне нельзя напрямую присваивать новое значение переменной, нужно выполнить дополнительный действия,
// чтобы зафиксировать изменение значения. Для этой цели данная страница содержит функции - сеттеры, 
// устанавливающие значения переменных.

// Добавление ключа (параметра) в список изменившихся параметров, чьи новые значения необходимо отправить на сервер
void addKeyToChanged(String key);
// DM manualMode
void set_manualMode(bool value);
// PD autoplayTime
void set_autoplayTime(uint32_t value);
// IT idleTime
void set_idleTime(uint32_t value);
// AL isAlarming 
void set_isAlarming(bool value);
// AL isAlarmStopped
void set_isAlarmStopped(bool value);

// EF thisMode
// EN thisMode
// UE thisMode
// UT thisMode
// UC thisMode
// SE thisMode
// BE thisMode
// SS thisMode
// SQ thisMode
void set_thisMode(int8_t value);
// SQ
void set_EffectScaleParam2(byte effect, byte value);
// OM memoryAvail
void set_memoryAvail(uint16_t value);
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
// AW alarmWeekDay
void set_alarmWeekDay(byte value);
// AE alarmEffect
void set_alarmEffect(byte value);
// MD alarmDuration
void set_alarmDuration(byte value);
// AT alarmHour[], alarmMinute[]
void set_alarmTime(byte wd, byte hour_value, byte minute_value);
// AU useSoftAP
void set_useSoftAP(bool value);
// AM1T AM1_hour
void set_AM1_hour(byte value);
// AM1T AM1_minute
void set_AM1_minute(byte value);
// AM1A AM1_effect_id
void set_AM1_effect_id(int8_t value);
// AM2T AM2_hour
void set_AM2_hour(byte value);
// AM2T AM2_minute
void set_AM2_minute(byte value);
// AM2A AM2_effect_id
void set_AM2_effect_id(int8_t value);
// AM3T AM3_hour
void set_AM3_hour(byte value);
// AM3T AM3_minute
void set_AM3_minute(byte value);
// AM3A AM3_effect_id
void set_AM3_effect_id(int8_t value);
// AM4T AM4_hour
void set_AM4_hour(byte value);
// AM4T AM4_minute
void set_AM4_minute(byte value);
// AM4A AM4_effect_id
void set_AM4_effect_id(int8_t value);
// AM5A dawn_effect_id
void set_dawn_effect_id(int8_t value);
// AM6A dusk_effect_id
void set_dusk_effect_id(int8_t value);


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
