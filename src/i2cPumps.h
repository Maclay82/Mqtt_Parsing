#include <Arduino.h>
#include "def_soft.h"     // Определение параметров эффектов, переменных программы и т.п.

#ifndef I2CPUMPS_H
#define I2CPUMPS_H
#ifdef PHTDSCONTROL
class i2cPumps
{
  public:
    i2cPumps (boolean revers);     // объявление класса с указанием адреса адаптера
    boolean  init (uint16_t volume, uint8_t num);
    void  putPumpScale(float koef, uint8_t num);
    float getPumpScale (uint8_t num);
    boolean  pourVol (uint16_t volume, uint8_t num);
    void  pourCalVol (uint16_t volume, uint8_t num);
    float  returnScaleCalVol (uint16_t volume, uint8_t num);
    uint8_t getPumpCount();
    boolean  getinit ();
  private:
    boolean           onpump;
//    IoAbstractionRef  &I2CmotorExp;
    float             scaleCal[PUMPCOUNT];
    uint16_t          CalVol[PUMPCOUNT];
};
#endif
#endif