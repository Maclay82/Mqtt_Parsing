#include "i2cPumps.h"
#ifdef PHTDSCONTROL
i2cPumps::i2cPumps(byte address, boolean revers) {
  onpump = revers;
  Wire.begin();
  I2CExp    = ioFrom8574(address);//0x20);     //Pumps
  for(int i = 0; i <= PUMPCOUNT-1; i++ ){ 
    ioDevicePinMode(I2CExp, i, OUTPUT);
    ioDeviceDigitalWrite(I2CExp, i, !onpump);
  }
  for(int i = 0; i <= PUMPCOUNT-1; i++ ){
    scaleCal[i]=10;
  }
  ioDeviceSync(I2CExp);
}

boolean   i2cPumps::pourVol (uint16_t volume, uint8_t num){
  boolean result = false;
  Serial.print(num);
  Serial.print(" ");
  Serial.print(volume);
  Serial.print(" "); 
  Serial.println(scaleCal[num-1]);

  if(num >= 1 && num <= PUMPCOUNT){
    result = ioDeviceDigitalWriteS(I2CExp, num-1, onpump);
    delay (volume*scaleCal[num-1]);
    result = ioDeviceDigitalWriteS(I2CExp, num-1, !onpump);
  }
  return result;
}

void i2cPumps::pourCalVol (uint16_t volume, uint8_t num) {
  CalVol[num-1] = volume;
  if(num>0 && num<=PUMPCOUNT){
    ioDeviceDigitalWriteS(I2CExp, num-1, onpump);
    delay (CalVol[num-1]*scaleCal[num-1]);
    ioDeviceDigitalWriteS(I2CExp, num-1, !onpump);
  }
}

float i2cPumps::returnScaleCalVol (uint16_t volume, uint8_t num) {
  if(num >= 1) {
    if(scaleCal[num-1]<=0) scaleCal[num - 1] = 1;
    scaleCal[num - 1] = (float)(scaleCal[num - 1] * CalVol[num - 1] / volume);
  }
  return scaleCal[num - 1];
}

void i2cPumps::putPumpScale  (float value, uint8_t num){
  scaleCal[num] = value;
}

float i2cPumps::getPumpScale (uint8_t num){
  return scaleCal[num];
}

uint8_t i2cPumps::getPumpCount(){
  return uint8_t(PUMPCOUNT);
}
#endif