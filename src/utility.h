#ifndef UTILITY_H
#define UTILITY_H         // служебные функции
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

uint32_t CountTokens(String str, char separator);
String GetToken(String &str, uint32_t index, char separator);
String padNum(int16_t num, byte cnt);       //сервисная функция вывода в строку
String getDateTimeString(time_t t);         //вывод даты и времени в строку
String getTimeString(time_t t);             //вывод времени в строку
// leap year calulator expects year argument as years offset from 1970
boolean LEAP_YEAR(uint16_t Y);
void sendNTPpacket(IPAddress& address);
void parseNTP();
void getNTP();

void profpub();
void calPointPub();
void HWprofPub();
boolean statusPub();    //Публикация состояния параметров системы

#ifdef PHTDSCONTROL
boolean setCollector(); //Приведение конфигурации коллектора в силу
#endif

void startWiFi(unsigned long waitTime);
void startSoftAP();
void connectToNetwork();

#ifdef CO2CONTROL
bool CO2Control(int cur);
int  CO2Check (int check);
bool TimeChk (int ON, int OFF);
#endif


void set_thisMode(int8_t value);

// useDHCP
void set_useDHCP(boolean value);
// useNtp
void set_useNtp(boolean value);
// syncTimePeriod
void set_syncTimePeriod(uint16_t value);
// timeZoneOffset
void set_timeZoneOffset(int16_t value);
// ntpServerName
void set_ntpServerName(String value);
// ssid
void set_Ssid(String value);
// pass
void set_pass(String value);
// apName
void set_SoftAPName(String value);

// pPass
void set_SoftAPPass(String value);
// IP wifi_connected
void set_wifi_connected(boolean value);
// IP IP_STA[]
void set_StaticIP(byte p1, byte p2, byte p3, byte p4);
// useSoftAP
void set_useSoftAP(boolean value);

#if (USE_MQTT == 1)
// useMQTT
void set_useMQTT(boolean value);

// mqtt_port
void set_mqtt_port(int16_t value);
// mqtt_server
void set_MqttServer(String value);
// mqtt_user
void set_MqttUser(String value);
// mqtt_pass
void set_MqttPass(String value);
// mqtt_send_delay
void set_mqtt_send_delay(int16_t value);
// mqtt_prefix
void set_MqttPrefix(String value);
// mqtt_state_packet
void set_mqtt_state_packet(boolean value);
// upTimeSendInterval
void set_upTimeSendInterval(uint16_t value);
#endif



#endif