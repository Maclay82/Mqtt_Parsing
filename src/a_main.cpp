#include <Arduino.h>
#include "a_def_hard.h"     // Определение параметров матрицы, пинов подключения и т.п
#include "a_def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.
#include "a_main.h"     
#include "eeprom1.h"     
#include "mqtt.h"
#include "setters.h"
#include "utility.h"
// ----------------------------------------------------

eModes  parseMode; // Текущий режим парсера
eSources cmdSource; // Источник команды; NONE - нет значения; BOTH - любой, UDP-клиент, MQTT-клиент


// Контроль времени цикла
// uint32_t last_ms = millis();  

void process() {  

  // Время прохода одного цикла
  /*
  uint16_t duration = millis() - last_ms;
  if (duration > 0) {
    Serial.print(F("duration="));
    Serial.println(duration);
  }
  */

  // принимаем данные
  parsing();

  // на время принятия данных матрицу не обновляем!
  if (!parseStarted) 
  {

    // Раз в час выполнять пересканирование текстов бегущих строк на наличие события непрерывного отслеживания.
    // При сканировании события с нечеткими датами (со звездочками) просматриваются не далее чем на сутки вперед
    // События с более поздним сроком не попадут в отслеживание. Поэтому требуется периодическое перестроение списка.
    // Сканирование требуется без учета наличия соединения с интернетом и значения флага useNTP - время может быть установлено вручную с телефона

    if (wifi_connected) {

      // Если настройки программы предполагают синхронизацию с NTP сервером и время пришло - выполнить синхронизацию
      if (useNtp) {
        if ((ntp_t > 0) && getNtpInProgress && (millis() - ntp_t > 5000)) {
          Serial.println(F("Таймаут NTP запроса!"));
          ntp_cnt++;
          getNtpInProgress = false;
          if (init_time && ntp_cnt >= 10) {
            Serial.println(F("Не удалось установить соединение с NTP сервером."));  
            refresh_time = false;
          }
          
          #if (USE_MQTT == 1)
          DynamicJsonDocument doc(256);
          String out;
          doc["act"]         = F("TIME");
          doc["server_name"] = ntpServerName;
          doc["server_ip"]   = timeServerIP.toString();
          doc["result"]      = F("TIMEOUT");
          serializeJson(doc, out);      
          SendMQTT(out, TOPIC_TME);
          #endif
        }
        
        bool timeToSync = ntpSyncTimer.isReady();
        if (timeToSync) { ntp_cnt = 0; refresh_time = true; }
        if (timeToSync || (refresh_time && (ntp_t == 0 || (millis() - ntp_t > 60000)) && (ntp_cnt < 10 || !init_time))) {
          ntp_t = millis();
          getNTP();
        }
      }

    }
    
    ///clockTicker();
    
    ///checkAlarmTime();
    ///checkAutoMode1Time();
    ///checkAutoMode2Time();
    ///checkAutoMode3Time();
    ///checkAutoMode4Time();

    butt.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
    // byte clicks = 0;
/*
    // Один клик
    if (butt.isSingle()) clicks = 1;    
    // Двойной клик
    if (butt.isDouble()) clicks = 2;
    // Тройной клик
    if (butt.isTriple()) clicks = 3;
    // Четверной и более клик
    if (butt.hasClicks()) clicks = butt.getClicks();
    
    if (butt.isPress()) {
      // Состояние - кнопку нажали  
    }
    
    if (butt.isRelease()) {
      // Состояние - кнопку отпустили
      isButtonHold = false;
    }
    
    if (butt.isHolded()) {
      isButtonHold = true;
      if (globalBrightness == 255)
        brightDirection = false;
      else if (globalBrightness == 0)  
        brightDirection = true;
      else  
        brightDirection = !brightDirection;
    }

    if (clicks > 0) {
      Serial.print(F("Кнопка нажата "));  
      Serial.print(String(clicks));
      Serial.println(F(" раз"));  
    }
    // Одинарный клик - включить . выключить панель
    if (clicks == 1) {
      if (isTurnedOff) {
        // Если выключен - включить панель, восстановив эффект на котором панель была выключена
        if (saveSpecialMode && saveSpecialModeId != 0) 
          setSpecialMode(saveSpecialModeId);
        else {
          saveMode = getCurrentManualMode();
          //if (saveMode == 0) set_globalColor(0xFFFFFF);
          Serial.println(String(F("Вкл: ")) + String(saveMode));
          setManualModeTo(getAutoplay());
          setEffect(saveMode);
        }
      } else {
        // Выключить панель, запомнив текущий режим
        saveMode = thisMode;
        bool mm = manualMode;
        // Выключить панель - черный экран
        putCurrentManualMode(saveMode);
        putAutoplay(mm);
        Serial.println(String(F("Выкл: ")) + String(saveMode));
      }
    }
    
    // Прочие клики работают только если не выключено
    if (isTurnedOff) {
        // Удержание кнопки повышает / понижает яркость панели (лампы)
        if (isButtonHold && butt.isStep()) {
          processButtonStep();
        }
    }
*/    

    // Если есть несохраненные в EEPROM данные - сохранить их
    if (saveSettingsTimer.isReady()) {
      saveSettings();
    }
  }
}

/*
void processButtonStep() {
  if (brightDirection) {
    if (globalBrightness < 10) set_globalBrightness(globalBrightness + 1);
    else if (globalBrightness < 250) set_globalBrightness(globalBrightness + 5);
    else {
      set_globalBrightness(255);
    }
  } else {
    if (globalBrightness > 15) set_globalBrightness(globalBrightness - 5);
    else if (globalBrightness > 1) set_globalBrightness(globalBrightness - 1);
    else {
      set_globalBrightness(1);
    }
  }
}
*/
// ********************* ПРИНИМАЕМ ДАННЫЕ **********************

void parsing() {
  
  // ****************** ОБРАБОТКА *****************
  String str, str1, str2;
  byte b_tmp;
  int8_t tmp_eff;
  // char c = 0;
  bool err = false;
  byte alarmHourVal;
  byte alarmMinuteVal;

  /*
    Протокол связи, посылка начинается с режима. Режимы:
    3 - управление играми из приложение WiFi Panel Player
      - $3 0;   включить на устройстве демо-режим
      - $3 10 - кнопка вверх
      - $3 11 - кнопка вправо
      - $3 12 - кнопка вниз
      - $3 13 - кнопка влево
      - $3 14 - центральная кнопка джойстика (ОК)

    6 - текст $6 N|some text, где N - назначение текста;

        1 - имя сервера NTP
        2 - SSID сети подключения
        3 - пароль для подключения к сети 
        4 - имя точки доступа
        5 - пароль к точке доступа
        6 - настройки будильников
        7 - строка запрашиваемых параметров для процедуры getStateString(), например - "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
        8 - имя сервера MQTT
        9 - имя пользователя MQTT
       10 - пароль к MQTT-серверу

       13 - префикс топика сообщения к MQTT-серверу

    11 - Настройки MQTT-канала (см. также $6 для N=8,9,10)
      - $11 1 X;   - использовать управление через MQTT сервер X; 0 - не использовать; 1 - использовать
      - $11 2 D;   - порт MQTT
      - $11 4 D;   - Задержка между последовательными обращениями к MQTT серверу
      - $11 5;     - Разорвать подключение к MQTT серверу, чтобы он иог переподключиться с новыми параметрами
      - $11 6 X;   - Флаг - отправка состояний 0 - индивидуально 1 - пакетом
      - $11 7 D;   - интервал отправки uptime на MQTT сервер в секундах или 0, если отключено
    14 - быстрая установка ручных режимов с пред-настройками
       - $14 0;  Черный экран (выкл);  
       - $14 1;  Белый экран (освещение);  
       - $14 2;  Цветной экран;  
       - $14 3;  Огонь;  
       - $14 4;  Конфетти;  
       - $14 5;  Радуга;  
       - $14 6;  Матрица;  
       - $14 7;  Светлячки;  
       - $14 8;  Часы ночные;
       - $14 9;  Часы бегущей строкой;
       - $14 10; Часы простые;  
    15 - скорость $15 скорость таймер; 0 - таймер эффектов
    16 - Режим смены эффектов: $16 value; N: 0 - ручной режим;  1 - авторежим; 2 - PrevMode; 3 - NextMode; 5 - вкл/выкл случайный выбор следующего режима
    17 - Время автосмены эффектов и бездействия: $17 сек сек;
    18 - Запрос текущих параметров программой: $18 page; page - страница настройки в программе на смартфоне (1..7) или специальный параметр (91..99)
         page 1:  // Настройки
         page 2:  // Эффекты
         page 3:  // Настройки бегущей строки
         page 4:  // Настройки часов
         page 5:  // Настройки будильника
         page 6:  // Настройки подключения
         page 7:  // Настройки режимов автовключения по времени
         page 10: // Загрузка картинок
         page 11: // Рисование
         page 12: // Игры

         page 95: // Ответ состояния будильника - сообщение по инициативе сервера
         page 96: // Ответ демо-режима звука - сообщение по инициативе сервера
         page 99: // Запрос списка эффектов
    19 - работа с настройками часов
      - $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
      - $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс

      - $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM

      - $19 17 D I; - Продолжительность отображения даты / часов (в секундах)
    20 - настройки и управление будильников
      - $20 0;       - отключение будильника (сброс состояния isAlarming)
     21 - настройки подключения к сети / точке доступа
      - $21 0 X - использовать точку доступа: X=0 - не использовать X=1 - использовать
      - $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
      - $21 2; Выполнить переподключение к сети WiFi
    22 - настройки включения режимов матрицы в указанное время
       - $22 HH1 MM1 NN1 HH2 MM2 NN2 HH3 MM3 NN3 HH4 MM4 NN4
             HHn - час срабатывания
             MMn - минуты срабатывания
             NNn - эффект: -3 - выключено; -2 - выключить матрицу; -1 - ночные часы; 0 - случайный режим и далее по кругу; 1 и далее - список режимов EFFECT_LIST 
    23 - прочие настройки
       - $23 0 VAL  - лимит по потребляемому току
       - $23 1 ST   - Сохранить EEPROM в файл    ST = 0 - внутр. файл. систему; 1 - на SD-карту
       - $23 2 ST   - Загрузить EEPROM из файла  ST = 0 - внутр. файл. системы; 1 - на SD-карты
  */  

  // Если прием данных завершен и управляющая команда в intData[0] распознана
  if (recievedFlag && intData[0] > 0 && intData[0] <= 23) {
    recievedFlag = false;

    switch (intData[0]) {

      // ----------------------------------------------------
      // 6 - прием строки: строка принимается в формате N|text, где N:
      //   0 - принятый текст бегущей строки $6 0|X|text - X - 0..9,A..Z - индекс строки
      //   1 - имя сервера NTP
      //   2 - имя сети (SSID)
      //   3 - пароль к сети
      //   4 - имя точки доступа
      //   5 - пароль точки доступа
      //   6 - настройки будильника в формате $6 6|DD EF WD HH1 MM1 HH2 MM2 HH3 MM3 HH4 MM4 HH5 MM5 HH6 MM6 HH7 MM7        
      //   7 - строка запрашиваемых параметров для процедуры getStateString(), например - "$6 7|CE CC CO CK NC SC C1 DC DD DI NP NT NZ NS DW OF"
      //   8 - имя сервера MQTT
      //   9 - имя пользователя MQTT
      //  10 - пароль к MQTT-серверу
      //  11 - картинка построчно $6 11|Y colorHEX X|colorHEX X|...|colorHEX X;
      //  12 - картинка по колонкам $6 12|X colorHEX Y|colorHEX Y|...|colorHEX Y;   - пока не реализовано (зарезервировано)
      //  13 - префикс топика сообщения к MQTT-серверу
      //  14 - текст бегущей строки для немедленного отображения без сохранения $6 14|text
      //  15 - Загрузить пользовательскую картинку из файла на матрицу; $6 15|ST|filename; ST - "FS" - файловая система; "SD" - карточка
      //  16 - Сохранить текущее изображение с матрицы в файл $6 16|ST|filename; ST - "FS" - файловая система; "SD" - карточка
      //  17 - Удалить файл $6 16|ST|filename; ST - "FS" - файловая система; "SD" - карточка
      // ----------------------------------------------------

      case 6:
        b_tmp = 0;
        tmp_eff = receiveText.indexOf("|");
        if (tmp_eff > 0) {
           b_tmp = receiveText.substring(0, tmp_eff).toInt();
           str = receiveText.substring(tmp_eff+1, receiveText.length()+1);
           switch(b_tmp) {

            case 0:
///              rescanTextEvents();

            case 1:
              set_ntpServerName(str);
              if (wifi_connected) {
                refresh_time = true; ntp_t = 0; ntp_cnt = 0;
              }
              break;

            case 2:
              set_Ssid(str);
              break;

            case 3:
              set_pass(str);
              break;

            case 4:
              set_SoftAPName(str);
              break;

            case 5:
              set_SoftAPPass(str);
              // Передается в одном пакете - использовать SoftAP, имя точки и пароль
              // После получения пароля - перезапустить создание точки доступа
              if (useSoftAP) startSoftAP();
              break;
              
            case 6:
              // Настройки будильника в формате $6 6|DD EF WD AD HH1 MM1 HH2 MM2 HH3 MM3 HH4 MM4 HH5 MM5 HH6 MM6 HH7 MM7
              // DD    - установка продолжительности рассвета (рассвет начинается за DD минут до установленного времени будильника)
              // EF    - установка эффекта, который будет использован в качестве рассвета
              // WD    - установка дней пн-вс как битовая маска
              // AD    - продолжительность "звонка" сработавшего будильника
              // HHx   - часы дня недели x (1-пн..7-вс)
              // MMx   - минуты дня недели x (1-пн..7-вс)
              //
              // Остановить будильник, если он сработал

              set_isAlarming(false);
              set_isAlarmStopped(false);

              // Настройки содержат 18 элементов (см. формат выше)
              tmp_eff = CountTokens(str, ' ');
              if (tmp_eff == 18) {
              
//                set_dawnDuration(constrain(GetToken(str, 1, ' ').toInt(),1,59));
                set_alarmEffect(GetToken(str, 2, ' ').toInt());
                set_alarmWeekDay(GetToken(str, 3, ' ').toInt());
                set_alarmDuration(constrain(GetToken(str, 4, ' ').toInt(),1,10));
                
                for(byte i=0; i<7; i++) {
                  alarmHourVal = constrain(GetToken(str, i*2+5, ' ').toInt(), 0, 23);
                  alarmMinuteVal = constrain(GetToken(str, i*2+6, ' ').toInt(), 0, 59);
                  set_alarmTime(i+1, alarmHourVal, alarmMinuteVal);
                }
              }
              break;
              
            case 7:
              // Запрос значений параметров, требуемых приложением вида str="CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
              // Каждый запрашиваемый приложением параметр - для заполнения соответствующего поля в приложении 
              // Передать строку для формирования, затем отправить параметры в приложение
              if (cmdSource == UDP) {
                str = "$18 " + getStateString(str) + ";";
              } else {
                #if (USE_MQTT == 1)
                // Если ключи разделены пробелом - заменить на пайпы '|'
                // Затем добавить в строку измененных параметров changed_keys
                // На следующей итерации параметры из строки changed_keys будут отправлены в канал MQTT
                str.replace(" ","|");
                int16_t pos_start = 0;
                int16_t pos_end = str.indexOf('|', pos_start);
                int16_t len = str.length();
                if (pos_end < 0) pos_end = len;
                while (pos_start < len && pos_end >= pos_start) {
                  if (pos_end > pos_start) {      
                    String key = str.substring(pos_start, pos_end);
                    if (key.length() > 0) addKeyToChanged(key);
                  }
                  pos_start = pos_end + 1;
                  pos_end = str.indexOf('|', pos_start);
                  if (pos_end < 0) pos_end = len;
                }
                #endif
              }
              break;
              
            #if (USE_MQTT == 1)
            case 8:
              set_MqttServer(str);
              break;
            case 9:
              set_MqttUser(str);
              break;
            case 10:
              set_MqttPass(str);
              break;
            case 13:
              set_MqttPrefix(str);
              break;
            #endif
           }
        }

        // При сохранении текста бегущей строки (b_tmp == 0) не нужно сразу автоматически сохранять ее в EEPROM - Сохранение будет выполнено по таймеру. 
        // При получении запроса параметров (b_tmp == 7) ничего сохранять не нужно - просто отправить требуемые параметры
        // При получении очередной строки изображения (b_tmp == 11 или b_tmp == 12) ничего сохранять не нужно
        // При получении текста без сохранения (b_tmp == 14) ничего сохранять не нужно
        // При загрузка / сохранение картинки (b_tmp == 15 или b_tmp == 16) ничего сохранять не нужно
        // Остальные полученные строки - сохранять сразу, ибо это настройки сети, будильники и другая критически важная информация
        switch (b_tmp) {
          case 0:
          case 7:
          case 11:
          case 12:
          case 14:
          case 15:
          case 16:
          case 17:
            // Ничего делать не нужно
            break;
          default:  
            saveSettings();
            break;
        }
        
        if (b_tmp >= 0 && b_tmp <= 17) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            switch (b_tmp) {
              case 0: 
                sendPageParams(3, cmdSource);
                break;
              case 6: 
                sendPageParams(5, cmdSource);
                break;
              case 7: 
              case 11:
              case 12:
              case 15:
              case 16:
              case 17:
                sendStringData(str, cmdSource);
                break;
              default:
                sendAcknowledge(cmdSource);
                break;
            }
          } else {
            switch (b_tmp) {
              case 11:
              case 12:
              case 15:
              case 16:
              case 17:
                sendStringData(str, cmdSource);
                break;
              default:
                // Другие команды - отправить подтверждение о выполнении
                sendAcknowledge(cmdSource);
                break;
            }
          }
        } else {
          #if (USE_MQTT == 1)
          notifyUnknownCommand(incomeBuffer);
          #endif
        }        
        break;

      // ----------------------------------------------------
      // 11 - Настройки MQTT-канала
      // - $11 1 X;   - использовать управление через MQTT сервер X; 0 - не использовать; 1 - использовать
      // - $11 2 D;   - Порт MQTT
      // - $11 4 D;   - Задержка между последовательными обращениями к MQTT серверу
      // - $11 5;     - Разорвать подключение к MQTT серверу, чтобы он иог переподключиться с новыми параметрами
      // - $11 6 X;   - Флаг - отправка состояний 0 - индивидуально 1 - пакетом
      // - $11 7 D;   - интервал отправки uptime на MQTT сервер в секундах или 0, если отключено
      // ----------------------------------------------------

      #if (USE_MQTT == 1)
      case 11:
         switch (intData[1]) {
           case 1:               // $11 1 X; - Использовать канал MQTT: 0 - нет; 1 - да
             set_useMQTT(intData[2] == 1);
             // Если MQTT канал только что включили - отправить туда все начальные настройки,
             // т.к. пока канал был отключен - состояние параметров изменялось, но сервер об этом не знает
             if (useMQTT) mqttSendStartState();
             break;
           case 2:               // $11 2 D; - Порт MQTT
             set_mqtt_port(intData[2]);
             break;
           case 4:               // $11 4 D; - Задержка между последовательными обращениями к MQTT серверу
             set_mqtt_send_delay(intData[2]);
             break;
           case 5:               // $11 5;   - Сохранить изменения ипереподключиться к MQTT серверу
             saveSettings();
             mqtt.disconnect();
             // Если подключаемся к серверу с другим именем и/или на другом порту - 
             // простой вызов 
             // mqtt.setServer(mqtt_server, mqtt_port)
             // не срабатывает - соединяемся к прежнему серверу, который был обозначен при старте программы
             // Единственный вариант - программно перезагрузить контроллер. После этого новый сервер подхватывается
             if (last_mqtt_server != String(mqtt_server) || last_mqtt_port != mqtt_port) {              
               ESP.restart();
             }
             // MQTT сервер мог поменять свои настройки, переключились на другой сервер или другой аккаунт - отправить туда все начальные настройки,
             if (useMQTT) mqttSendStartState();
             break;
           case 6:               // $11 6 X; - Отправка параметров состояния в MQTT: 0 - индивидуально; 1 - пакетом
             set_mqtt_state_packet(intData[2] == 1);
             break;
           case 7:               // $11 7 D; - Интервал отправки uptime на сервер MQTT
             set_upTimeSendInterval(intData[2]);
             break;
          default:
            err = true;
            notifyUnknownCommand(incomeBuffer);
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          sendAcknowledge(cmdSource);
        }
        break;
      #endif
      
      // ----------------------------------------------------
      // 18 - Запрос текущих параметров программой: $18 page;
      //    page 1:   // Настройки
      //    page 2:   // Эффекты
      //    page 3:   // Настройки бегущей строки
      //    page 4:   // Настройки часов
      //    page 5:   // Настройки будильника
      //    page 6:   // Настройки подключения
      //    page 7:   // Настройки режимов автовключения по времени
      //    page 10:  // Загрузка картинок
      //    page 11:  // Рисование
      //    page 12:  // Игры
      //    page 91:  // Запрос текста бегущих строк как есть без обработки макросов
      //    page 92:  // Запрос списков звуков бегущей строки - макрос {A}
      //    page 93:  // Зпрос списка звуков будильника
      //    page 94:  // Запрос списка звуков рассвета
      //    page 95:  // Ответ состояния будильника - сообщение по инициативе сервера
      //    page 96:  // Ответ демо-режима звука - сообщение по инициативе сервера
      //    page 99:  // Запрос списка эффектов
      // ----------------------------------------------------

      case 18: 
        if (intData[1] == 0) { // ping
        } else {                          // запрос параметров страницы приложения
          // Для команд, пришедших от MQTT отправлять ответы так же как и для команд, полученных из UDP
          sendPageParams(intData[1], cmdSource);
        }
        break;

      // ----------------------------------------------------
      // 19 - работа с настройками часов
      //   $19 1 X; - сохранить настройку X "Часы в эффектах" (общий, для всех эффектов)
      //   $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
      //   $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
      //   $19 4 X; - Выключать индикатор TM1637 при выключении экрана X: 0 - нет, 1 - да
      //   $19 5 X; - Режим цвета часов оверлея X: 0,1,2,3
      //   $19 6 X; - Ориентация часов  X: 0 - горизонтально, 1 - вертикально
      //   $19 7 X; - Размер часов X: 0 - авто, 1 - малые 3х5, 2 - большие 5x7
      //   $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
      //   $19 9 X; - Показывать температуру вместе с малыми часами 1 - да; 0 - нет
      //   $19 10 X; - Цвет ночных часов:  0 - R; 1 - G; 2 - B; 3 - C; 3 - M; 5 - Y; 6 - W;
      //   $19 11 X; - Яркость ночных часов:  0..255
      //   $19 12 X; - скорость прокрутки часов оверлея или 0, если часы остановлены по центру
      //   $19 16 X; - Показывать дату в режиме часов  X: 0 - нет, 1 - да
      //   $19 17 D I; - Продолжительность отображения даты / часов (в секундах)
      // ----------------------------------------------------
      
      case 19: 
         switch (intData[1]) {
           case 2:               // $19 2 X; - Использовать синхронизацию часов NTP  X: 0 - нет, 1 - да
             set_useNtp(intData[2] == 1);
             if (wifi_connected) {
               refresh_time = true; ntp_t = 0; ntp_cnt = 0;
             }
             break;
           case 3:               // $19 3 N Z; - Период синхронизации часов NTP и Часовой пояс
             set_SYNC_TIME_PERIOD(intData[2]);
             set_timeZoneOffset((int8_t)intData[3]);
             ntpSyncTimer.setInterval(1000L * 60 * SYNC_TIME_PERIOD);
             if (wifi_connected) {
               refresh_time = true; ntp_t = 0; ntp_cnt = 0;
             }
             break;
           case 8:               // $19 8 YYYY MM DD HH MM; - Установить текущее время YYYY.MM.DD HH:MM
             setTime(intData[5],intData[6],0,intData[4],intData[3],intData[2]);
             init_time = true; refresh_time = false; ntp_cnt = 0;
            //  rescanTextEvents();
             break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] != 8) {
              sendPageParams(4, cmdSource);
            } else {
              sendAcknowledge(cmdSource);
            }
          } else {
            sendAcknowledge(cmdSource);
          }
        }
        break;

      // ----------------------------------------------------
      //  20 - настройки и управление будильников
      // - $20 0;       - отключение будильника (сброс состояния isAlarming)
      // ----------------------------------------------------
      
      case 20:
        switch (intData[1]) { 
          case 0:  
            // $20 0;       - отключение будильника (сброс состояния isAlarming)
             if (isAlarming) //stopAlarm();            
            break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] == 0) {
              sendPageParams(5, cmdSource);
            } else if (intData[1] == 1 || intData[1] == 2) { // Режимы установки параметров - сохранить
              sendPageParams(5, cmdSource);
            } else {
              sendPageParams(96);
            }        
          } else {
            sendAcknowledge(cmdSource);
          }          
        }
        break;

      // ----------------------------------------------------
      // 21 - настройки подключения к сети / точке доступа
      //   $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
      //   $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
      //   $21 2; Выполнить переподключение к сети WiFi
      // ----------------------------------------------------

      case 21:
        // Настройки подключения к сети
        switch (intData[1]) { 
          // $21 0 0 - не использовать точку доступа $21 0 1 - использовать точку доступа
          case 0:  
            set_useSoftAP(intData[2] == 1);
            if (useSoftAP && !ap_connected) 
              startSoftAP();
            else if (!useSoftAP && ap_connected) {
              if (wifi_connected) { 
                ap_connected = false;              
                WiFi.softAPdisconnect(true);
                Serial.println(F("Точка доступа отключена."));
              }
            }      
            break;
          case 1:  
            // $21 1 IP1 IP2 IP3 IP4 - установить статический IP адрес подключения к локальной WiFi сети, пример: $21 1 192 168 0 106
            // Локальная сеть - 10.х.х.х или 172.16.х.х - 172.31.х.х или 192.168.х.х
            // Если задан адрес не локальной сети - сбросить его в 0.0.0.0, что означает получение динамического адреса 
            if (!(intData[2] == 10 || (intData[2] == 172 && intData[3] >= 16 && intData[3] <= 31) || (intData[2] == 192 && intData[3] == 168))) {
              set_StaticIP(0, 0, 0, 0);
            }
            set_StaticIP(intData[2], intData[3], intData[4], intData[5]);
            break;
          case 2:  
            // $21 2; Выполнить переподключение к сети WiFi
            saveSettings();
            delay(10);
            startWiFi(5000);     // Время ожидания подключения 5 сек
            // showCurrentIP(true);
            break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        if (!err) {
          // Для команд, пришедших от MQTT отправлять только ACK;
          // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
          if (cmdSource == UDP) {
            if (intData[1] == 0 || intData[1] == 1) {
              sendAcknowledge(cmdSource);
            } else {
              sendPageParams(6, cmdSource);
            }
          } else {
            sendAcknowledge(cmdSource);
          }
        }
        break;

      // ----------------------------------------------------
      // 22 - настройки включения режимов матрицы в указанное время NN5 - Действие на "Рассвет", NN6 - действие на "Закат"
      // - $22 HH1 MM1 NN1 HH2 MM2 NN2 HH3 MM3 NN3 HH4 MM4 NN4 NN5 NN6
      //     HHn - час срабатывания
      //     MMn - минуты срабатывания
      //     NNn - эффект: -3 - выключено; -2 - выключить матрицу; -1 - ночные часы; 0 - случайный режим и далее по кругу; 1 и далее - список режимов EFFECT_LIST 
      // ----------------------------------------------------

      case 22:
        set_AM1_hour(intData[1]);
        set_AM1_minute(intData[2]);
        set_AM1_effect_id(intData[3]);

        set_AM2_hour(intData[4]);
        set_AM2_minute(intData[5]);
        set_AM2_effect_id(intData[6]);

        set_AM3_hour(intData[7]);
        set_AM3_minute(intData[8]);
        set_AM3_effect_id(intData[9]);

        set_AM4_hour(intData[10]);
        set_AM4_minute(intData[11]);
        set_AM4_effect_id(intData[12]);

        saveSettings();
        
        // Для команд, пришедших от MQTT отправлять только ACK;
        // Для команд, пришедших от UDP отправлять при необходимости другие данные, например - состояние элементов управления на странице от которой пришла команда 
        if (cmdSource == UDP) {
          sendPageParams(7, cmdSource);
        } else {
          sendAcknowledge(cmdSource);
        }
        break;

      // ----------------------------------------------------
      // 23 - прочие настройки
      // - $23 0 VAL  - лимит по потребляемому току
      // ----------------------------------------------------

      case 23:
        // $23 0 VAL - лимит по потребляемому току
        // $23 1 ST   - Сохранить EEPROM в файл    ST = 0 - внутр. файл. систему; 1 - на SD-карту
        // $23 2 ST   - Загрузить EEPROM из файла  ST = 0 - внутр. файл. системы; 1 - на SD-карты
        switch(intData[1]) {
          case 1:
            err = !saveEepromToFile(intData[2] == 1 ? "SD" : "FS");
            if (err) {
              str = F("$18 ER:[E~Не удалось сохранить резервную копию настроек]|EE:");
            } else {
              str = F("$18 ER:[I~Резервная копия настроек создана]|EE:");
            }
            str += String(eeprom_backup) + ";";
            sendStringData(str, cmdSource);
            break;
          case 2:
            err = !loadEepromFromFile(intData[2] == 1 ? "SD" : "FS");
            if (err) {
              str = F("$18 ER:[E~Не удалось загрузить резервную копию настроек];");
            } else {
              str = F("$18 ER:[I~Настройки из резервной копии восстановлены];");
            }
            sendStringData(str, cmdSource);
            // Если настройки загружены без ошибок - перезагрузить устройство
            if (!err) {
              delay(500);
              ESP.restart();
            }
            break;
          default:
            err = true;
            #if (USE_MQTT == 1)
            notifyUnknownCommand(incomeBuffer);
            #endif
            break;
        }
        break;

      // ----------------------------------------------------
      default:
        #if (USE_MQTT == 1)
        notifyUnknownCommand(incomeBuffer);
        #endif
        break;

    }
  }

  // ****************** ПАРСИНГ *****************

  // Если предыдущий буфер еще не разобран - новых данных из сокета не читаем, продолжаем разбор уже считанного буфера
  haveIncomeData = bufIdx > 0 && bufIdx < packetSize; 

  #if (USE_MQTT == 1)
  if (!haveIncomeData) {
    // Есть ли поступившие по каналу MQTT команды?
    if (queueLength > 0) {
      String command = cmdQueue[queueReadIdx++];
      if (queueReadIdx >= QSIZE_IN) queueReadIdx = 0;
      queueLength--;
      
      cmdSource = MQTT;
      haveIncomeData = true;
      bufIdx = 0;
      packetSize = command.length();
      memcpy(incomeBuffer, command.c_str(), packetSize);

      Serial.print(F("MQTT пакeт размером "));
      Serial.println(packetSize);
    }
  }
  #endif
  
  if (!haveIncomeData) {
    packetSize = udp.parsePacket();      
    haveIncomeData = packetSize > 0;      
  
    if (haveIncomeData) {                
      // read the packet into packetBufffer
      int len = udp.read(incomeBuffer, BUF_MAX_SIZE);
      if (len > 0) {          
        incomeBuffer[len] = 0;
      }
      bufIdx = 0;

      cmdSource = UDP;
      
      delay(0);            // ESP8266 при вызове delay отрабатывает стек IP протокола, дадим ему поработать        

      Serial.print(F("UDP << ip='"));
      IPAddress remote = udp.remoteIP();
      Serial.print(remote.toString());
      Serial.print(":");
      Serial.print(udp.remotePort());
      Serial.print("'");
      if (udp.remotePort() == localPort) {
        Serial.print(F("; cmd='"));
        Serial.print(incomeBuffer);
        Serial.print("'");
      }
      if (udp.remotePort() == 123) {
        Serial.print(F("; ntp sync"));
      }
      Serial.println();

      Serial.print(F("UDP пакeт размером "));
      Serial.println(packetSize);
    }

    // NTP packet from time server
    if (haveIncomeData && udp.remotePort() == 123) {
      parseNTP();
      haveIncomeData = false;
      bufIdx = 0;      
    }
  }

  if (haveIncomeData) {         

    // Из-за ошибки в компоненте UdpSender в Thunkable - теряются половина отправленных 
    // символов, если их кодировка - двухбайтовый UTF8, т.к. оно вычисляет длину строки без учета двухбайтовости
    // Чтобы символы не терялись - при отправке строки из андроид-программы, она добивается с конца пробелами
    // Здесь эти конечные пробелы нужно предварительно удалить
    while (packetSize > 0 && incomeBuffer[packetSize-1] == ' ') packetSize--;
    incomeBuffer[packetSize] = 0;

    if (parseMode == TEXT) {                         // если нужно принять строку - принимаем всю

      // Оставшийся буфер преобразуем с строку
      if (intData[0] == 6) {  // текст
        receiveText = String(&incomeBuffer[bufIdx]);
        receiveText.trim();
      }
                
      incomingByte = ending;                       // сразу завершаем парс
      parseMode = NORMAL;
      bufIdx = 0; 
      packetSize = 0;                              // все байты из входящего пакета обработаны
    } else {
      incomingByte = incomeBuffer[bufIdx++];       // обязательно ЧИТАЕМ входящий символ
    } 
  }       
    
  if (haveIncomeData) {

    if (parseStarted) {                                             // если приняли начальный символ (парсинг разрешён)
      if (incomingByte != divider && incomingByte != ending) {      // если это не пробел И не конец
        string_convert += incomingByte;                             // складываем в строку
      } 
      else 
      {                                                      // если это пробел или ; конец пакета
        if (parse_index == 0) {
          byte cmdMode = string_convert.toInt();
          intData[0] = cmdMode;
//          if (cmdMode == 6) {
//            parseMode = TEXT;
//          }
//          else 
          parseMode = NORMAL;
        }

        if (parse_index == 1) {       // для второго (с нуля) символа в посылке
          if (parseMode == NORMAL) intData[parse_index] = string_convert.toInt();           // преобразуем строку в int и кладём в массив}
//          if (parseMode == COLOR) {                                                         // преобразуем строку HEX в цифру
//             set_globalColor((uint32_t)HEXtoInt(string_convert));
//            if (intData[0] == 0) {
//              incomingByte = ending;
//              parseStarted = false;
//            } else {
//              parseMode = NORMAL;
//            }
//          }
        } 
        else {
          intData[parse_index] = string_convert.toInt();  // преобразуем строку в int и кладём в массив
        }
        string_convert = "";                        // очищаем строку
        parse_index++;                              // переходим к парсингу следующего элемента массива
      }
    }

    if (incomingByte == header) {                   // если это $
      parseStarted = true;                          // поднимаем флаг, что можно парсить
      parse_index = 0;                              // сбрасываем индекс
      string_convert = "";                          // очищаем строку
    }

    if (incomingByte == ending) {                   // если таки приняли ; - конец парсинга
      parseMode = NORMAL;
      parseStarted = false;                         // сброс
      recievedFlag = true;                          // флаг на принятие
      bufIdx = 0;
    }

    if (bufIdx >= packetSize) {                     // Весь буфер разобран 
      bufIdx = 0;
      packetSize = 0;
    }
  }
}

void sendPageParams(int page) {
  sendPageParams(page, BOTH);
}

void sendPageParams(int page, eSources src) {

  String str = "", color, text;
  // CRGB c1, c2;
  // int8_t tmp_eff = -1;
  bool err = false;
  
  switch (page) { 
    case 1:  // Настройки
      str = getStateString("W|H|DM|PS|PD|IT|AL|RM|PW|BR|WU|WT|WR|WS|WC|WN|WZ|SD|FS|EE");
      break;
    case 2:  // Эффекты
      str = getStateString("EF|EN|UE|UT|UC|SE|SS|BE|SQ");
      break;
    case 4:  // Настройки часов
      str = getStateString("CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF|TM");
      break;
    case 5:  // Настройки будильника
      str = getStateString("AL|AW|AT|AD|AE|MX|MU|MD|MV|MA|MB|MP");
      break;
    case 6:  // Настройки подключения
      str = getStateString("AU|AN|AA|NW|NA|IP|QZ|QA|QP|QS|QU|QW|QD|QR|QK|UI");
      break;
    case 7:  // Настройки режимов автовключения по времени
      str = getStateString("WZ|WU|AM1T|AM1A|AM2T|AM2A|AM3T|AM3A|AM4T|AM4A|AM5A|AM6A");
      break;
    case 91:  // Запрос текста бегущей строки для редактирования указанной ячейки или замены строки текста в списке ячейки
      str = getStateString("TS|TY");
      addKeyToChanged("TY");       // Отправить также строку в канал MQTT
      addKeyToChanged("TS");       // Тескт в строке мог быть изменен - отправить в канал MQTT состояние строк
      break;
     
//    case 95:  // Ответ состояния будильника - сообщение по инициативе сервера
//      str = getStateString("AL");
//      cmd95 = str;
//      src = BOTH;
//      break;
    case 98:  // Запрос всех строк текста бегущей строки
      str = getStateString("LT");
      break;
    case 99:  // Запрос списка эффектов
      str = getStateString("LE");
      break;
    default:
      err = true;
      #if (USE_MQTT == 1)
      DynamicJsonDocument doc(256);
      String out;
      doc["message"] = F("unknown page");
      doc["text"]    = String(F("нет страницы с номером ")) + String(page);
      serializeJson(doc, out);      
      SendMQTT(out, TOPIC_ERR);
      #endif
      break;
  }

  if (!err) {
    if (str.length() > 0) {
      // Отправить клиенту запрошенные параметры страницы / режимов
      str = "$18 " + str + ";";
      sendStringData(str, src);
    } else {
      sendAcknowledge(cmdSource);
    }
  }
}

void sendStringData(String &str, eSources src) {
  #if (USE_MQTT == 1)
  if (src == MQTT || src == BOTH) {
    SendMQTT(str, TOPIC_DTA);
  }
  #endif
  if (src == UDP || src == BOTH) {
    int max_text_size = sizeof(incomeBuffer);        // Размер приемного буфера формирования текста загружаемой / отправляемой строки
    memset(incomeBuffer, '\0', max_text_size);
    str.toCharArray(incomeBuffer, str.length() + 1);        
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*) incomeBuffer, str.length()+1);
    udp.endPacket();
    delay(0);
    Serial.println(String(F("UDP ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(incomeBuffer));
  }
}

String getStateValue(String &key, int8_t effect, JsonVariant* value) {

  // W:число     ширина матрицы
  // H:число     высота матрицы
  // AA:[текст]  пароль точки доступа
  // AD:число    продолжительность рассвета, мин
  // AE:число    эффект, использующийся для будильника
  // AO:X        включен будильник 0-нет, 1-да
  // AL:X        сработал будильник 0-нет, 1-да
  // AM1T:HH MM  час 00..23 и минуты 00..59 включения режима 1, разделенные пробелом:      "AM1T:23 15"
  // AM1A:NN     номер эффекта режима 1:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM2T:HH MM  час 00..23 и минуты 00..59 включения режима 1, разделенные пробелом:      "AM2T:23 15"
  // AM2A:NN     номер эффекта режима 2:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM3T:HH MM  час 00..23 и минуты 00..59 включения режима 1, разделенные пробелом:      "AM3T:23 15"
  // AM3A:NN     номер эффекта режима 3:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы; 0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM4T:HH MM  час 00..23 и минуты 00..59 включения режима 1, разделенные пробелом:      "AM4T:23 15"
  // AM4A:NN     номер эффекта режима 4:   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM5A:NN     номер эффекта режима по времени "Рассвет":   -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AM6A:NN     номер эффекта режима по времени "Закат":     -3 - не используется; -2 - выключить матрицу; -1 - ночные часы;  0 - включить случайный с автосменой; 1 - номер режима из списка EFFECT_LIST
  // AN:[текст]  имя точки доступа
  // AT: DW HH MM  часы-минуты времени будильника для дня недели DW 1..7 -> например "AT:1 09 15"
  // AU:X        создавать точку доступа 0-нет, 1-да
  // AW:число    битовая маска дней недели будильника b6..b0: b0 - пн .. b7 - вс
  // BE:число    контрастность эффекта
  // BR:число    яркость
  // C1:цвет     цвет режима "монохром" часов оверлея; цвет: 192,96,96 - R,G,B
  // C2:цвет     цвет режима "монохром" бегущей строки; цвет: 192,96,96 - R,G,B
  // CС:X        режим цвета часов оверлея: 0,1,2
  // CE:X        оверлей часов вкл/выкл, где Х = 0 - выкл; 1 - вкл (использовать часы в эффектах)
  // CK:X        размер горизонтальных часов, где Х = 0 - авто; 1 - малые 3x5; 2 - большие 5x7 
  // CL:X        цвет рисования в формате RRGGBB
  // CO:X        ориентация часов: 0 - горизонтально, 1 - вертикально
  // CT:X        режим цвета текстовой строки: 0,1,2
  // DC:X        показывать дату вместе с часами 0-нет, 1-да
  // DD:число    время показа даты при отображении часов (в секундах)
  // DI:число    интервал показа даты при отображении часов (в секундах)
  // DM:Х        демо режим, где Х = 0 - ручное управление; 1 - авторежим
  // DW:X        показывать температуру вместе с малыми часами 0-нет, 1-да
  // EE:X        Наличие сохраненных настроек EEPROM на SD-карте или в файловой системе МК: 0 - нет 1 - есть в FS; 2 - есть на SD; 3 - есть в FS и на SD
  // EF:число    текущий эффект - id
  // EN:[текст]  текущий эффект - название
  // ER:[текст]  отправка клиенту сообщения инфо/ошибки последней операции (WiFiPanel - сохр. резервной копии настроекж WiFiPlayer - сообщение операции с изображением)
  // FS:X        доступность внутренней файловой системы микроконтроллера для хранения файлов: 0 - нет, 1 - да
  // FL0:[список] список файлов картинок нарисованных пользователем с внутренней памяти, разделенный запятыми, ограничители [] обязательны
  // FL1:[список] список файлов картинок нарисованных пользователем с SD-карты, разделенный запятыми, ограничители [] обязательны
  // IP:xx.xx.xx.xx Текущий IP адрес WiFi соединения в сети
  // IT:число    время бездействия в секундах
  // LE:[список] список эффектов, разделенный запятыми, ограничители [] обязательны
  // LF:[список] список файлов эффектов с SD-карты, разделенный запятыми, ограничители [] обязательны
  // LT:[список] список текстовых строк, разделенных '~', ограничители [] обязательны
  // MA:число    номер файла звука будильника из SD:/01
  // MB:число    номер файла звука рассвета из SD:/02
  // MD:число    сколько минут звучит будильник, если его не отключили
  // MP:папка~файл  номер папки и файла звука который проигрывается, номер папки и звука разделены '~'
  // MU:X        использовать звук в будильнике 0-нет, 1-да
  // MV:число    максимальная громкость будильника
  // MX:X        MP3 плеер доступен для использования 0-нет, 1-да
  // NA:[текст]  пароль подключения к сети
  // NB:Х        яркость цвета ночных часов, где Х = 1..255
  // NС:Х        цвет ночных часов, где Х = 0 - R; 1 - G; 2 - B; 3 - C; 4 - M; 5 - Y;
  // NP:Х        использовать NTP, где Х = 0 - выкл; 1 - вкл
  // NS:[текст]  сервер NTP, ограничители [] обязательны
  // NT:число    период синхронизации NTP в минутах
  // NW:[текст]  SSID сети подключения
  // NZ:число    часовой пояс -12..+12
  // OF:X        выключать часы вместе с лампой 0-нет, 1-да
  // OM:X        сколько ячеек осталось свободно для хранения строк
  // PD:число    продолжительность режима в секундах
  // PS:X        состояние программного вкл/выкл панели 0-выкл, 1-вкл
  // PW:число    ограничение по току в миллиамперах
  // QA:X        использовать MQTT 0-нет, 1-да
  // QD:число    задержка отправки сообщения MQTT
  // QP:число    порт подключения к MQTT серверу
  // QK:X        пакетная отправка состояний в MQTT-канал 0 - каждое состояние отправляется индивидуално в свой топик, соответствующий названию параметра, 1 - состояние отправляется сборными пакетами 
  // QR:X        префикс для формирования топика
  // QS:[text]   имя MQTT сервера, например QS:[srv2.clusterfly.ru]
  // QU:[text]   имя пользователя MQTT соединения, например QU:[user_af7cd12a]
  // QW:[text]   пароль MQTT соединения, например QW:[pass_eb250bf5]
  // QZ:X        сборка поддерживает MQTT 0-нет, 1-да
  // RM:Х        смена режимов в случайном порядке, где Х = 0 - выкл; 1 - вкл
  // S1:[список] список звуков будильника, разделенный запятыми, ограничители [] обязательны        
  // S2:[список] список звуков рассвета, разделенный запятыми, ограничители [] обязательны        
  // S3:[список] список звуков для макроса {A} бегущей строки, ограничители [] обязательны        
  // SC:число    скорость смещения часов оверлея
  // SD:X        наличие и доступность SD карты: Х = 0 - нат SD карты; 1 - SD карта доступна
  // SE:число    скорость эффектов
  // SS:число    параметр #1 эффекта
  // SQ:спец     параметр #2 эффекта; спец - "L>val>itrm1,item2,..itemN" - список, где val - текущее, далее список; "C>x>title" - чекбокс, где x=0 - выкл, x=1 - вкл; title - текст чекбокса
  // ST:число    скорость смещения бегущей строки
  // TE:X        оверлей текста бегущей строки вкл/выкл, где Х = 0 - выкл; 1 - вкл (использовать бегущую строку в эффектах)
  // TI:число    интервал отображения текста бегущей строки
  // TS:строка   строка состояния кнопок выбора текста из массива строк: 36 символов 0..5, где
  //               0 - серый - пустая
  //               1 - черный - отключена
  //               2 - зеленый - активна - просто текст? без макросов
  //               3 - голубой - активна, содержит макросы кроме даты
  //               4 - синий - активная, содержит макрос даты
  //               5 - красный - для строки 0 - это управляющая строка
  // TY:[Z:текст] текст для строки, с указанным индексом I 0..35, Z 0..9,A..Z. Ограничители [] обязательны; текст ответа в формате: 'I:Z > текст'; 
  // TZ:[Z:текст] То же, что 'TY". Служит для фоновой загрузки всего массива сохраненных строк в смартфон для формирования элементов списка выбора строки. Получив этот ответ приложение на смартфоне берет следующий индекс и отправляет команду `'$13 3 I;'` для получения следующей строки.
  // UC:X        использовать часы поверх эффекта 0-нет, 1-да
  // UE:X        использовать эффект в демо-режиме 0-нет, 1-да
  // UP:число    uptime системы в секундах
  // UT:X        использовать бегущую строку поверх эффекта 0-нет, 1-да
  // W1          текущая погода ('ясно','пасмурно','дождь'и т.д.)
  // W2          текущая температура
  // WC:X        Использовать цвет для отображения температуры в дневных часах  X: 0 - выключено; 1 - включено
  // WN:X        Использовать цвет для отображения температуры в ночных часах  X: 0 - выключено; 1 - включено
  // WR:число    Регион погоды Yandex
  // WS:число    Регион погоды OpeenWeatherMap
  // WT:число    Период запроса сведений о погоде в минутах
  // WU:X        Использовать получение погоды с сервера: 0 - выключено; 1 - включено

  String str = "", tmp;
  

  // Ручной / Авто режим
  if (key == "DM") {
    if (value) {
      value->set(!manualMode);
      return String(!manualMode);
    }
    return str + "DM:" + String(!manualMode);
  }

  // Продолжительность режима в секундах
  if (key == "PD") {
    if (value) {
      value->set(autoplayTime / 1000);
      return String(autoplayTime / 1000);
    }
    return str + "PD:" + String(autoplayTime / 1000); 
  }

  // Время бездействия в минутах
  if (key == "IT") {
    if (value) {
      value->set(idleTime / 60 / 1000);
      return String(idleTime / 60 / 1000);
    }
    return str + "IT:" + String(idleTime / 60 / 1000);
  }

  // Сработал будильник 0-нет, 1-да
  if (key == "AL") {
    if (value) {
      value->set((isAlarming) && !isAlarmStopped);
      return String((isAlarming) && !isAlarmStopped);
    }
    return str + "AL:" + String(((isAlarming) && !isAlarmStopped)); 
  }

  // Текущий эффект - id
  if (key == "EF") {
    if (value) {
      value->set(effect);
      return String(effect);
    }
    return str + "EF:" + String(effect+1); // +1 т.к эффекты считаются с нуля, а индекс в списке эффектов - с 1
  }

  // Сколько ячеек осталось свободно для хранения строк
  if (key == "OM") {
    if (value) {
      value->set(memoryAvail);
      return String(memoryAvail);
    }
    return str + "OM:" + String(memoryAvail);
  }

  // // Строка состояния заполненности строк текста
  // if (key == "TS") {
  //   tmp = getTextStates();
  //   if (value) {
  //     value->set(tmp);
  //     return tmp;
  //   }
  //   return str + "TS:" + tmp;
  // }

  // Использовать получение времени с интернета
  if (key == "NP") {
    if (value) {
      value->set(useNtp);
      return String(useNtp);
    }
    return str + "NP:" + String(useNtp);
  }

  // Период синхронизации NTP в минутах
  if (key == "NT") {
    if (value) {
      value->set(SYNC_TIME_PERIOD);
      return String(SYNC_TIME_PERIOD); 
    }
    return str + "NT:" + String(SYNC_TIME_PERIOD); 
  }

  // Часовой пояс
  if (key == "NZ") {
    if (value) {
      value->set(timeZoneOffset);
      return String(timeZoneOffset);
    }
    return str + "NZ:" + String(timeZoneOffset);
  }

  // Имя сервера NTP (url)
  if (key == "NS") {
    tmp = String(ntpServerName);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "NS:" + "[" + tmp + "]";
  }

  // Продолжительность рассвета, мин
  if (key == "AD") {
    if (value) {
      value->set(dawnDuration);
      return String(dawnDuration);
    }
    return str + "AD:" + String(dawnDuration);
  }

 
  // Эффект применяемый в рассвете: Индекс в списке в приложении смартфона начинается с 1
  if (key == "AE") {
    if (value) {
      value->set(alarmEffect);
      return String(alarmEffect);
    }
    return str + "AE:" + String(alarmEffect + 1);
  }

  // uptime - время работы системы в секундах
  if (key == "UP") {
    uint32_t upt = upTime;
    if (upt > 0) {
      upt = ((uint32_t)now()) - upt;
    }
    if (value) {
      value->set(upt);
      return String(upt);  
    }
    return str + "UP:" + String(upt);
  }

  // создавать точку доступа
  if (key == "AU") {
    if (value) {
      value->set(useSoftAP);
      return String(useSoftAP);  
    }
    return str + "AU:" + String(useSoftAP);
  }

  // Имя точки доступа
  if (key == "AN") {
    tmp = String(apName);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AN:" + "[" + tmp + "]";
  }

  // Пароль точки доступа
  if (key == "AA") {
    tmp = String(apPass);
    if (value) {
      // value->set(tmp);  // Решено пароль по открытому каналу на [публичный] сервер MQTT не отправлять
      return "";           // tmp;
    }
    return str + "AA:" + "[" + tmp + "]";
  }

  // Имя локальной сети (SSID)
  if (key == "NW") {
    tmp = String(ssid);
    if (value){
      value->set(tmp);
      return tmp;
    }
    return str + "NW:" + "[" + tmp + "]";
  }

  // Пароль к сети
  if (key == "NA") {
    tmp = String(pass);
    if (value) {
      // value->set(tmp);  // Решено пароль по открытому каналу на [публичный] сервер MQTT не отправлять
      return "";           // tmp;
    }
    return str + "NA:" + "[" + tmp + "]";
  }

  // IP адрес
  if (key == "IP") {
    tmp = String(wifi_connected ? WiFi.localIP().toString() : "");
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "IP:" + tmp;
  }

  // Время Режима №1
  if (key == "AM1T") {
    tmp = padNum(AM1_hour,2) + " " + padNum(AM1_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM1T:" + tmp;
  }

  // Действие Режима №1
  if (key == "AM1A") {
    if (value) {
      value->set(AM1_effect_id);
      return String(AM1_effect_id);
    }
    return str + "AM1A:" + String(AM1_effect_id);
  }

  // Время Режима №2
  if (key == "AM2T") {
    tmp = padNum(AM2_hour,2) + " " + padNum(AM2_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM2T:" + tmp;
  }

  // Действие Режима №2
  if (key == "AM2A") {
    if (value) {
      value->set(AM2_effect_id);
      return String(AM2_effect_id);
    }
    return str + "AM2A:" + String(AM2_effect_id);
  }

  // Время Режима №3
  if (key == "AM3T") {
    tmp = padNum(AM3_hour,2) + " " + padNum(AM3_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM3T:" + tmp;
  }

  // Действие Режима №3
  if (key == "AM3A") {
    if (value) {
      value->set(AM3_effect_id);
      return String(AM3_effect_id);
    }
    return str + "AM3A:" + String(AM3_effect_id);
  }

  // Время Режима №4
  if (key == "AM4T") {
    tmp = padNum(AM4_hour,2) + " " + padNum(AM4_minute,2);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "AM4T:" + tmp;
  }

  // Действие Режима №4
  if (key == "AM4A") {
    if (value) {
      value->set(AM4_effect_id);
      return String(AM4_effect_id);
    }
    return str + "AM4A:" + String(AM4_effect_id);
  }

  // Действие Режима "Рассвет"
  if (key == "AM5A") {
    if (value) {
      value->set(dawn_effect_id);
      return String(dawn_effect_id);
    }
    return str + "AM5A:" + String(dawn_effect_id);
  }

  // Действие Режима "Закат"
  if (key == "AM6A") {
    if (value) {
      value->set(dusk_effect_id);
      return String(dusk_effect_id);
    }
    return str + "AM6A:" + String(dusk_effect_id);
  }

  // Наличие резервной копии EEPROM
  if (key == "EE") {
    if (value) {
      value->set(eeprom_backup);
      return String(eeprom_backup);
    }
    return str + "EE:" + String(eeprom_backup); 
  }

  // Доступность внутренней файловой системы
  if (key == "FS") {
    if (value) {
      value->set(spiffs_ok);
      return String(spiffs_ok);
    }
    return str + "FS:" + String(spiffs_ok); 
  }

  // Список эффектов прошивки
  if (key == "LE") {
    tmp = String(EFFECT_LIST);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "LE:" + "[" + tmp.substring(0,BUF_MAX_SIZE-12) + "]"; 
  }

  // Прошивка поддерживает MQTT 0-нет, 1-да
  if (key == "QZ") {
    if (value) {
      value->set(USE_MQTT == 1);
      return String(USE_MQTT == 1);
    }
    return str + "QZ:" + String(USE_MQTT == 1);  
  }

#if (USE_MQTT == 1)

  // Использовать MQTT 0-нет, 1-да
  if (key == "QA") {
    if (value) {
      value->set(useMQTT);
      return String(useMQTT);  
    }
    return str + "QA:" + String(useMQTT);  
  }

  // QP:число    порт подключения к MQTT серверу
  if (key == "QP") {
    if (value) {
      value->set(mqtt_port);
      return String(mqtt_port);  
    }
    return str + "QP:" + String(mqtt_port);  
  }

  // QS:[text]   имя MQTT сервера, например QS:[srv2.clusterfly.ru]
  if (key == "QS") {
    tmp = String(mqtt_server);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "QS:" + "[" + tmp +  "]";
  }
  
  // QU:[text]   имя пользователя MQTT соединения, например QU:[user_af7cd12a]
  if (key == "QU") {
    tmp = String(mqtt_user);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "QU:" + "[" + tmp +  "]";
  }

  // QW:[text]   пароль MQTT соединения, например QW:[pass_eb250bf5]
  if (key == "QW") {
    tmp = String(mqtt_pass);
    if (value) {
      // value->set(tmp);  // Решено пароль по открытому каналу на [публичный] сервер MQTT не отправлять
      return "";           // tmp;
    }
    return str + "QW:" + "[" + tmp +  "]";
  }

  // QD:число    задержка между отправками сообщений к MQTT серверу
  if (key == "QD") {
    if (value) {
      value->set(mqtt_send_delay);
      return String(mqtt_send_delay);
    }
    return str + "QD:" + String(mqtt_send_delay);  
  }

  // QR:[text]   префикс топика MQTT сообщения, например QR:[user_af7cd12a/WiFiPanel-0]
  if (key == "QR") {
    tmp = String(mqtt_prefix);
    if (value) {
      value->set(tmp);
      return tmp;
    }
    return str + "QR:" + "[" + tmp +  "]";
  }

  // Использовать отправку состояния параметров в MQTT пакетами 0-индивидуально, 1-пакетами
  if (key == "QK") {
    if (value) {
      value->set(mqtt_state_packet);
      return String(mqtt_state_packet);  
    }
    return str + "QK:" + String(mqtt_state_packet);
  }

  // Интервал отправки uptime на cервер в секундах
  if (key == "UI") {
    if (value) {
      value->set(upTimeSendInterval);
      return String(upTimeSendInterval);  
    }
    return str + "UI:" + String(upTimeSendInterval);
  }
#endif

  // Запрошенный ключ не найден - вернуть пустую строку
  return "";
}

String getStateString(String keys) {
  String str = "", s_tmp, key;

  // Ключи буквы/цифры, разделенные пробелами или пайпами '|' 
  // Если строка ключей ограничена квадратными скобками или кавычками - удалить их;
  // В конце может быть ";" - не требуется - удалить ее (в середине ее быть не может)
  keys.replace(";","");
  keys.replace("[","");
  keys.replace("]","");
  keys.replace("\"","");

  // Ключи могут быть разделены '|' или пробелами
  keys.replace(" ","|");
  keys.replace("/r"," ");
  keys.replace("/n"," ");
  keys.trim();
  
  int8_t tmp_eff = -1;
  int16_t pos_start = 0;
  int16_t pos_end = keys.indexOf('|', pos_start);
  int16_t len = keys.length();
  if (pos_end < 0) pos_end = len;


  // Строка keys содержит ключи запрашиваемых данных, разделенных знаком '|', например "CE|CC|CO|CK|NC|SC|C1|DC|DD|DI|NP|NT|NZ|NS|DW|OF"
  while (pos_start < len && pos_end >= pos_start) {
    if (pos_end > pos_start) {      
      key = keys.substring(pos_start, pos_end);
      if (key.length() > 0) {
        s_tmp = getStateValue(key, tmp_eff);
        if (s_tmp.length() > 0) {
          str += s_tmp + "|";
        }
      }      
    }
    pos_start = pos_end + 1;
    pos_end = keys.indexOf('|', pos_start);
    if (pos_end < 0) pos_end = len;
  }

  len = str.length();
  if (len > 0 && str.charAt(len - 1) == '|') {
    str = str.substring(0, len - 1);
  }
  return str;
}

// Второй параметр эффекта thisMode для отправки на телефон параметра "SQ:"
String getParam2ForMode(byte mode) {
 // Эффекты не имеющие настройки вариации (параметр #2) отправляют значение "Х" - программа делает ползунок настройки недоступным 
 String str = "X"; 
 switch (mode) {
   case 0:
    break;
 }
 return str;   
}

void sendAcknowledge(eSources src) {
  if (src == UDP || src == BOTH) {
    // Отправить подтверждение, чтобы клиентский сокет прервал ожидание
    String reply = "";
    bool isCmd = false; 
    if (cmd95.length() > 0) { reply += cmd95; cmd95 = ""; isCmd = true;}
    if (cmd96.length() > 0) { reply += cmd96; cmd96 = ""; isCmd = true; }
    reply += "ack" + String(ackCounter++) + ";";  
    reply.toCharArray(replyBuffer, reply.length()+1);
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*) replyBuffer, reply.length()+1);
    udp.endPacket();
    delay(0);
    if (isCmd) {
      Serial.println(String(F("Ответ на ")) + udp.remoteIP().toString() + ":" + String(udp.remotePort()) + " >> " + String(replyBuffer));
    }
  }
}

void setEffect(byte eff) {

//  resetModes();
  if (eff >= MAX_EFFECT) {
    return;
  }
  
  set_thisMode(eff);

//  setTimersForMode(thisMode);  

  if (manualMode){
    putCurrentManualMode(thisMode);
  } else {
    autoplayTimer = millis();
  }
}

void setManualModeTo(bool isManual) {
  set_manualMode(isManual);
  
  idleState = !manualMode;
  if (idleTime == 0 || manualMode) {
    idleTimer.setInterval(4294967295);
  } else {
    idleTimer.setInterval(idleTime);    
  }
  idleTimer.reset();
  autoplayTimer = millis();
}

void parseNTP() {
  getNtpInProgress = false;
  Serial.println(F("Разбор пакета NTP"));
  ntp_t = 0; ntp_cnt = 0; init_time = true; refresh_time = false;
  unsigned long highWord = word(incomeBuffer[40], incomeBuffer[41]);
  unsigned long lowWord = word(incomeBuffer[42], incomeBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;    
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  unsigned long seventyYears = 2208988800UL ;
  unsigned long t = secsSince1900 - seventyYears + (timeZoneOffset) * 3600UL;
  String t2 = getDateTimeString(t);

  Serial.print(F("Секунд с 1970: "));
  Serial.println(t);
  Serial.print(F("Текущее время: ")); 
  Serial.println(t2);

  setTime(t);  
  // calculateDawnTime();
  // rescanTextEvents();

  // Если время запуска еще не определено - инициализировать его
  if (upTime == 0) {
    upTime = t - millis() / 1000L;
  }

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(256);
  String out;
  doc["act"] = F("TIME");
  doc["server_name"] = ntpServerName;
  doc["server_ip"] = timeServerIP.toString();
  doc["result"] = F("OK");
  doc["time"] = secsSince1900;
  doc["s_time2"] = t2;
  serializeJson(doc, out);      
  SendMQTT(out, TOPIC_TME);
  #endif
}

void getNTP() {
  if (!wifi_connected) return;
  WiFi.hostByName(ntpServerName, timeServerIP);
  IPAddress ip1, ip2;
  ip1.fromString(F("0.0.0.0"));
  ip2.fromString(F("255.255.255.255"));
  if (timeServerIP == ip1 || timeServerIP == ip2) {
    Serial.print(F("Не удалось получить IP aдрес сервера NTP -> "));
    Serial.print(ntpServerName);
    Serial.print(F(" -> "));
    Serial.println(timeServerIP);
    timeServerIP.fromString(F("85.21.78.91"));  // Один из ru.pool.ntp.org  // 91.207.136.55, 91.207.136.50, 46.17.46.226
    Serial.print(F("Используем сервер по умолчанию: "));
    Serial.println(timeServerIP);
  }
  getNtpInProgress = true;
  printNtpServerName();  
  // sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  ntp_t = millis();  

  #if (USE_MQTT == 1)
  DynamicJsonDocument doc(256);
  String out;
  doc["act"] = F("TIME");
  doc["server_name"] = ntpServerName;
  doc["server_ip"] = timeServerIP.toString();
  doc["result"] = F("REQUEST");
  serializeJson(doc, out);
  SendMQTT(out, TOPIC_TME);
  #endif
}