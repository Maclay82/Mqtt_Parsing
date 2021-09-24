#ifndef UTILITY_H
#define UTILITY_H// служебные функции
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

// hex string to uint32_t
uint32_t HEXtoInt(String hexValue);
// uint32_t to Hex string
String IntToHex(uint32_t value);
uint32_t CountTokens(String str, char separator);
String GetToken(String &str, uint32_t index, char separator);
String padNum(int16_t num, byte cnt);
String getDateTimeString(time_t t);
// leap year calulator expects year argument as years offset from 1970
bool LEAP_YEAR(uint16_t Y);
void sendNTPpacket(IPAddress& address);
void parseNTP();
void getNTP();

void profpub();
void CalprofPub();
void HWprofPub();
bool statusPub();    //Публикация состояния параметров системы
bool setCollector(); //Приведение конфигурации коллектора в силу
void startWiFi(unsigned long waitTime);
void startSoftAP();
void connectToNetwork();
#endif