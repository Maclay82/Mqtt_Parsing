// #include <Arduino.h>
// #include "Time.h"  
// служебные функции

// hex string to uint32_t
uint32_t HEXtoInt(String hexValue);

// uint32_t to Hex string
String IntToHex(uint32_t value);
uint32_t CountTokens(String str, char separator);
String GetToken(String &str, uint32_t index, char separator);

// ------------------------- CRC16 -------------------------

uint16_t getCrc16(uint8_t * data, uint16_t len);


String padNum(int16_t num, byte cnt);
// leap year calulator expects year argument as years offset from 1970
bool LEAP_YEAR(uint16_t Y);

String getDateTimeString(time_t t);

void printNtpServerName();