#ifndef A_MAIN_H
#define A_MAIN_H

void startWiFi(unsigned long waitTime);
void startSoftAP();
void process();

// ********************* ПРИНИМАЕМ ДАННЫЕ **********************

void parsing();

void sendPageParams(int page);

void sendPageParams(int page, eSources src);
void sendStringData(String &str, eSources src);
String getStateValue(String &key, int8_t effect, JsonVariant* value = nullptr);

String getStateString(String keys);
void sendAcknowledge(eSources src);

void parseNTP();
void getNTP();

String padNum(int16_t num, byte cnt);
String getDateTimeString(time_t t);

#endif