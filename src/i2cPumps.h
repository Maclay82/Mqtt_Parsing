#ifndef PUMPS_H
#define PUMPS_H

#define PUMPCOUNT 8

#include "def_soft.h"

class i2cPumps
{
  public:
    i2cPumps (byte address, bool revers);     // объявление класса с указанием адреса адаптера
    void  setPumpScale(float koef, uint8_t num);
    float getPumpScale (uint8_t num);
    bool  pourVol (uint16_t volume, uint8_t num);
    void  pourCalVol (uint16_t volume, uint8_t num);
    void  returnCalVol (uint16_t volume, uint8_t num);
    uint8_t getPumpCount();
  private:
    bool     onpump = true;
    float    kCal[PUMPCOUNT];
    uint16_t CalVol[PUMPCOUNT];
};

#endif