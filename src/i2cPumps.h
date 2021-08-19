#ifndef I2CPUMPS_H
#define I2CPUMPS_H

#define PUMPCOUNT 8

#include <Arduino.h>
#include <IoAbstraction.h>
#include <IoAbstractionWire.h>


class i2cPumps
{
  public:
    i2cPumps (byte address, bool revers);     // объявление класса с указанием адреса адаптера
    bool  init (uint16_t volume, uint8_t num);
    void  putPumpScale(float koef, uint8_t num);
    float getPumpScale (uint8_t num);
    bool  pourVol (uint16_t volume, uint8_t num);
    void  pourCalVol (uint16_t volume, uint8_t num);
    float  returnScaleCalVol (uint16_t volume, uint8_t num);
    uint8_t getPumpCount();
  private:
    bool  onpump;
    IoAbstractionRef I2CExp;
    float    scaleCal[PUMPCOUNT];
    uint16_t CalVol[PUMPCOUNT];
};

#endif