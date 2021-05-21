#include <Arduino.h>
#include "utility.h"

// служебные функции

// hex string to uint32_t
uint32_t HEXtoInt(String hexValue) {

  hexValue.toUpperCase();
  if (hexValue.charAt(0) == '#') {
    hexValue = hexValue.substring(1);
  }

  if (hexValue.startsWith("0X")) {
    hexValue = hexValue.substring(2);
  }

  byte tens, ones, number1, number2, number3;
  tens = (hexValue[0] <= '9') ? hexValue[0] - '0' : hexValue[0] - '7';
  ones = (hexValue[1] <= '9') ? hexValue[1] - '0' : hexValue[1] - '7';
  number1 = (16 * tens) + ones;

  tens = (hexValue[2] <= '9') ? hexValue[2] - '0' : hexValue[2] - '7';
  ones = (hexValue[3] <= '9') ? hexValue[3] - '0' : hexValue[3] - '7';
  number2 = (16 * tens) + ones;

  tens = (hexValue[4] <= '9') ? hexValue[4] - '0' : hexValue[4] - '7';
  ones = (hexValue[5] <= '9') ? hexValue[5] - '0' : hexValue[5] - '7';
  number3 = (16 * tens) + ones;

  return ((uint32_t)number1 << 16 | (uint32_t)number2 << 8 | number3 << 0);
}

// uint32_t to Hex string
String IntToHex(uint32_t value) {
  String sHex = "00000" + String(value, HEX);
  byte len = sHex.length();
  if (len > 6) {
    sHex = sHex.substring(len - 6);
    sHex.toUpperCase();
  }
  return sHex;
}

uint32_t CountTokens(String str, char separator) {

  uint32_t count = 0;
  int pos = 0;
  String l_str = str;

  l_str.trim();
  if (l_str.length() <= 0) return 0;
  pos = l_str.indexOf(separator);
  while (pos >= 0) {
    count++;
    pos = l_str.indexOf(separator, pos + 1);
  }
  return ++count;
}

String GetToken(String &str, uint32_t index, char separator) 
{

  uint32_t count = CountTokens(str, separator);

  if (count <= 1 || index < 1 || index > count) return str;

  uint32_t pos_start = 0;
  uint32_t pos_end = str.length();

  count = 0;
  for (uint32_t i = 0; i < pos_end; i++) 
  {
    if (str.charAt(i) == separator) {
      count++;
      if (count == index) {
        pos_end = i;
        break;
      } else {
        pos_start = i + 1;
      }
    }
  }
  return str.substring(pos_start, pos_end);
}


// ------------------------- CRC16 -------------------------

uint16_t getCrc16(uint8_t * data, uint16_t len)
{
    uint8_t lo;
    union
    {
        uint16_t value;
        struct { uint8_t lo, hi; } bytes;
    } crc;
 
    crc.value = 0xFFFF;
 
    while ( len-- )
    {
        lo = crc.bytes.lo;
        crc.bytes.lo = crc.bytes.hi;
        crc.bytes.hi = lo ^ *data++;
 
        uint8_t mask = 1;
 
        if ( crc.bytes.hi & mask ) crc.value ^= 0x0240;
        if ( crc.bytes.hi & ( mask << 1 ) ) crc.value ^= 0x0480;
        if ( crc.bytes.hi & ( mask << 2 ) ) crc.bytes.hi ^= 0x09;
        if ( crc.bytes.hi & ( mask << 3 ) ) crc.bytes.hi ^= 0x12;
        if ( crc.bytes.hi & ( mask << 4 ) ) crc.bytes.hi ^= 0x24;
        if ( crc.bytes.hi & ( mask << 5 ) ) crc.bytes.hi ^= 0x48;
        if ( crc.bytes.hi & ( mask << 6 ) ) crc.bytes.hi ^= 0x90;
        if ( crc.bytes.hi & ( mask << 7 ) ) crc.value ^= 0x2001;
    }
 
    return crc.value;
}

String padNum(int16_t num, byte cnt) {
  char data[12];
  String fmt = "%0"+ String(cnt) + "d";
  sprintf(data, fmt.c_str(), num);
  return String(data);
}

// leap year calulator expects year argument as years offset from 1970
bool LEAP_YEAR(uint16_t Y) {
  return ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) );
}

String getDateTimeString(time_t t) {
  // uint8_t hr = hour(t);
  // uint8_t mn = minute(t);
  // uint8_t sc = second(t);
  // uint8_t dy = day(t);
  // uint8_t mh = month(t);
  // uint16_t yr = year(t);
  uint8_t hr = 22;
  uint8_t mn = 22;
  uint8_t sc = 22;
  uint8_t dy = 23;
  uint8_t mh = 4;
  uint16_t yr = 2021;
  
  return padNum(dy,2) + "." + padNum(mh,2) + "." + padNum(yr,4) + " " + padNum(hr,2) + ":" + padNum(mn,2) + ":" + padNum(sc,2);  
}

void printNtpServerName() {
  Serial.print(F("NTP-сервер "));
  // Serial.print(ntpServerName);
  Serial.print(F(" -> "));
  // Serial.println(timeServerIP);
}