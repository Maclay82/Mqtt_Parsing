#include <Arduino.h>
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

// Для поддержки работы MQTT нужно знать какой параметр изменился, чтобы отправить изменения на сервер
// По этой причмне нельзя напрямую присваивать новое значение переменной, нужно выполнить дополнительный действия,
// чтобы зафиксировать изменение значения. Для этой цели данная страница содержит функции - сеттеры, 
// устанавливающие значения переменных.

// Добавление ключа (параметра) в список изменившихся параметров, чьи новые значения необходимо отправить на сервер
void addKeyToChanged(String key) {
  #if (USE_MQTT == 1)  
    String search = "|" + key + "|";
    // Добавляемый ключ есть в списке ключей, которыми интересуется клиент (STATE_KEYS)?
    if (String(STATE_KEYS).indexOf(search) >= 0) {
      // Если ключа еще нет в строке измененных параметров - добавить 
      if      (changed_keys.length() == 0)       changed_keys = search;
      else if (changed_keys.indexOf(search) < 0) changed_keys += key + "|";
    }
  #endif  
}
/*
// DM manualMode
void set_manualMode(bool value) {
  if (manualMode == value) return;
  putAutoplay(value);
  manualMode = getAutoplay();
  addKeyToChanged("DM");
}

// PD autoplayTime
void set_autoplayTime(uint32_t value) {
  if (autoplayTime == value) return;
  putAutoplayTime(value);
  autoplayTime = getAutoplayTime();
  addKeyToChanged("PD");
}

// IT idleTime
void set_idleTime(uint32_t value) {
  if (idleTime == value) return;;
  putIdleTime(value);
  idleTime = getIdleTime();
  addKeyToChanged("IT");
}

// AL isAlarming 
void set_isAlarming(bool value) {
  if (isAlarming == value) return;
  isAlarming = value;
  addKeyToChanged("AL");
}

// AL isAlarmStopped
void set_isAlarmStopped(bool value) {
  if (isAlarmStopped == value) return;
  isAlarmStopped = value;
  addKeyToChanged("AL");
}


// EF thisMode
// EN thisMode
// UE thisMode
// UT thisMode
// UC thisMode
// SE thisMode
// BE thisMode
// SS thisMode
// SQ thisMode
*/
void set_thisMode(int8_t value) {
  if (thisMode == value) return;
  
  // bool valid = (value == -1) || (value >= 0 && value < MAX_EFFECT);
  // if (!valid) return;

  // valid = (value >= 0 && value < MAX_EFFECT);

  // String keySE = "SE", keyBE = "BE", keyUT = "UT", keyUC = "UC", old_SQ, old_SS, old_SE, old_BE, old_UT, old_UC;

  thisMode = value;
  putCurrentMode(thisMode);
  setCollector(); //Приведение конфигурации коллектора в силу
  statusPub();
  if(thisMode%2 == 0 && thisMode != 0) count_mode = true;
  else count_mode = false;

  // addKeyToChanged("EF");
  // addKeyToChanged("EN");
}


// DI useDHCP
void set_useDHCP(bool value) {
  if (useDHCP == value) return;
  putUseDHCP(value);
  useDHCP = getUseDHCP();
  addKeyToChanged("DI");
}

// NP useNtp
void set_useNtp(bool value) {
  if (useNtp == value) return;
  putUseNtp(value);
  useNtp = getUseNtp();
  addKeyToChanged("NP");
}

// NT syncTimePeriod
void set_syncTimePeriod(uint16_t value) {
  if (syncTimePeriod == value) return;
  putNtpSyncTime(value);
  syncTimePeriod = getNtpSyncTime();
  addKeyToChanged("NT");
}

// NZ timeZoneOffset
void set_timeZoneOffset(int16_t value) {
  if (timeZoneOffset == value) return;
  putTimeZone(value);
  timeZoneOffset = getTimeZone();
  addKeyToChanged("NZ");
}

// NS ntpServerName
void set_ntpServerName(String value) {
  if (getNtpServer() == value) return;
  putNtpServer(value);  
  getNtpServer().toCharArray(ntpServerName, 31);
  addKeyToChanged("NS");
}

// NW ssid
void set_Ssid(String value) {
  if (getSsid() == value) return;
  putSsid(value);
  getSsid().toCharArray(ssid, 24);
  addKeyToChanged("NW");
}

// NA pass
void set_pass(String value) {
  if (getPass() == value) return;
  putPass(value);
  getPass().toCharArray(pass, 16);
  addKeyToChanged("NA");
}
              
// AN apName
void set_SoftAPName(String value) {
  if (getSoftAPName() == value) return;
  putSoftAPName(value);
  getSoftAPName().toCharArray(pass, 16);
  addKeyToChanged("AN");
}              

// AA apPass
void set_SoftAPPass(String value) {
  if (getSoftAPPass() == value) return;
  putSoftAPPass(value);
  getSoftAPPass().toCharArray(apPass, 16);
  addKeyToChanged("AA");
}              

// IP wifi_connected
void set_wifi_connected(bool value) {
  if (wifi_connected == value) return;
  wifi_connected = value;
  addKeyToChanged("IP");
}              

// IP IP_STA[]
void set_StaticIP(byte p1, byte p2, byte p3, byte p4) {
  IP_STA[0] = p1; 
  IP_STA[1] = p2; 
  IP_STA[2] = p3; 
  IP_STA[3] = p4; 
  putStaticIP(p1, p2, p3, p4);
  addKeyToChanged("IP");
}              
/*
// AW alarmWeekDay
void set_alarmWeekDay(byte value) {
  if (alarmWeekDay == value) return;
  putAlarmParams(value,dawnDuration,alarmEffect,alarmDuration);
  alarmWeekDay = getAlarmWeekDay();
  addKeyToChanged("AW");
}

// AE alarmEffect
void set_alarmEffect(byte value) {
  if (alarmEffect == value) return;
  // byte alarmWeekDay = getAlarmWeekDay();
//  putAlarmParams(alarmWeekDay,dawnDuration,value,alarmDuration);
  addKeyToChanged("AE");
}

// MD alarmDuration
void set_alarmDuration(byte value) {
  if (alarmDuration == value) return;
  // byte alarmWeekDay = getAlarmWeekDay();
//  putAlarmParams(alarmWeekDay,dawnDuration,alarmEffect,value);
  addKeyToChanged("MD");
}

// AT alarmHour[], alarmMinute[]
void set_alarmTime(byte wd, byte hour_value, byte minute_value) {
  byte old_hour   = getAlarmHour(wd);
  byte old_minute = getAlarmMinute(wd);
  if (old_hour == hour_value && old_minute == minute_value) return;
  putAlarmTime(wd, hour_value, minute_value);
  alarmHour[wd-1] = getAlarmHour(wd);
  alarmMinute[wd-1] = getAlarmMinute(wd);
  addKeyToChanged("AT");
}
*/
// AU useSoftAP
void set_useSoftAP(bool value) {
  if (useSoftAP == value) return;
  putUseSoftAP(value);
  useSoftAP = getUseSoftAP();
  addKeyToChanged("AU");
}
/*
// AM1T AM1_hour
void set_AM1_hour(byte value) {
  if (AM1_hour == value) return;
  putAM1hour(value);
  AM1_hour = getAM1hour();
  addKeyToChanged("AM1T");
}

// AM1T AM1_minute
void set_AM1_minute(byte value) {
  if (AM1_minute == value) return;
  putAM1minute(value);
  AM1_minute = getAM1minute();
  addKeyToChanged("AM1T");
}

// AM1A AM1_effect_id
void set_AM1_effect_id(int8_t value) {
  if (AM1_effect_id == value) return;
  putAM1effect(value);
  AM1_effect_id = getAM1effect();  
  addKeyToChanged("AM1A");
}

// AM2T AM2_hour
void set_AM2_hour(byte value) {
  if (AM2_hour == value) return;
  putAM2hour(value);
  AM2_hour = getAM2hour();
  addKeyToChanged("AM2T");
}

// AM2T AM2_minute
void set_AM2_minute(byte value) {
  if (AM2_minute == value) return;
  putAM2minute(value);
  AM2_minute = getAM2minute();
  addKeyToChanged("AM2T");
}

// AM2A AM2_effect_id
void set_AM2_effect_id(int8_t value) {
  if (AM2_effect_id == value) return;
  putAM2effect(value);
  AM2_effect_id = getAM2effect();
  addKeyToChanged("AM2A");
}

// AM3T AM3_hour
void set_AM3_hour(byte value) {
  if (AM3_hour == value) return;
  putAM3hour(value);
  AM3_hour = getAM3hour();
  addKeyToChanged("AM3T");
}

// AM3T AM3_minute
void set_AM3_minute(byte value) {
  if (AM3_minute == value) return;
  putAM3minute(value);
  AM3_minute = getAM3minute();
  addKeyToChanged("AM3T");
}

// AM3A AM3_effect_id
void set_AM3_effect_id(int8_t value) {
  if (AM3_effect_id == value) return;
  putAM3effect(value);
  AM3_effect_id = getAM3effect();
  addKeyToChanged("AM3A");
}

// AM4T AM4_hour
void set_AM4_hour(byte value) {
  if (AM4_hour == value) return;
  putAM4hour(value);
  AM4_hour = getAM4hour();
  addKeyToChanged("AM4T");
}

// AM4T AM4_minute
void set_AM4_minute(byte value) {
  if (AM4_minute == value) return;
  putAM4minute(value);
  AM4_minute = getAM4minute();
  addKeyToChanged("AM4T");
}

// AM4A AM4_effect_id
void set_AM4_effect_id(int8_t value) {
  if (AM4_effect_id == value) return;
  putAM4effect(value);
  AM4_effect_id = getAM4effect();
  addKeyToChanged("AM4A");
}

// AM5A dawn_effect_id
void set_dawn_effect_id(int8_t value) {
  if (dawn_effect_id == value) return;
  putAM5effect(value);
  dawn_effect_id = getAM5effect();
  addKeyToChanged("AM5A");
}

// AM6A dusk_effect_id
void set_dusk_effect_id(int8_t value) {
  if (dusk_effect_id == value) return;
  putAM6effect(value);
  dusk_effect_id = getAM6effect();
  addKeyToChanged("AM6A");
}
*/

#if (USE_MQTT == 1)
// QA useMQTT
void set_useMQTT(bool value) {
  if (useMQTT == value) return;  
  if (useMQTT || value) stopMQTT = false;
  putUseMqtt(value);
  useMQTT = getUseMqtt();
  
  addKeyToChanged("QA");
}

// QP mqtt_port
void set_mqtt_port(int16_t value) {
  if (mqtt_port == value) return;  
  putMqttPort(value);
  mqtt_port = getMqttPort();
  addKeyToChanged("QP");
}

// QS mqtt_server
void set_MqttServer(String value) {
  if (getMqttServer() == value) return;
  putMqttServer(value);
  getMqttServer().toCharArray(mqtt_server, 24);
  addKeyToChanged("QS");
}

// QU mqtt_user
void set_MqttUser(String value) {
  if (getMqttUser() == value) return;
  putMqttUser(value);
  getMqttUser().toCharArray(mqtt_user, 14);
  addKeyToChanged("QU");
}

// QW mqtt_pass
void set_MqttPass(String value) {
  if (getMqttPass() == value) return;
  putMqttPass(value);
  getMqttPass().toCharArray(mqtt_pass, 14);
  addKeyToChanged("QW");
}

// QD mqtt_send_delay
void set_mqtt_send_delay(int16_t value) {
  if (mqtt_send_delay == value) return;  
  putMqttSendDelay(value);
  mqtt_send_delay = getMqttSendDelay();
  addKeyToChanged("QD");
}

// QR mqtt_prefix
void set_MqttPrefix(String value) {
  if (getMqttPrefix() == value) return;
  putMqttPrefix(value);
  getMqttPrefix().toCharArray(mqtt_prefix, 30);
  addKeyToChanged("QR");
}

// QK mqtt_state_packet
void set_mqtt_state_packet(bool value) {
  if (mqtt_state_packet == value) return;  
  putSendStateInPacket(value);
  mqtt_state_packet = getSendStateInPacket();
  addKeyToChanged("QK");
}

// UI upTimeSendInterval
void set_upTimeSendInterval(uint16_t value) {
  if (upTimeSendInterval == value) return;;
  putUpTimeSendInterval(value);
  upTimeSendInterval = getUpTimeSendInterval();
  addKeyToChanged("UI");
}

#endif
