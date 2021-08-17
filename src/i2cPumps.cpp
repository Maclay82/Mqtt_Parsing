#include "i2cPumps.h"

i2cPumps::i2cPumps(byte address, bool revers) {
  onpump = revers;
  // Wire.beginTransmission(address); // transmit to device
  // Wire.write(byte(B00000000));            // sends instruction byte  
  // Wire.endTransmission();     // stop transmitting
//  IoAbstractionRef ioExp  = ioFrom8574(address);     //Pumps
  for(int i = 0; i <= 7; i++ ){ 
    ioDevicePinMode(ioExp, i, OUTPUT);
  }
  for(int i = 0; i <= 7; i++ ){
    ioDeviceDigitalWrite(ioExp, i, !onpump);
  }
  ioDeviceSync(ioExp);
}

void i2cPumps::pourCalVol (uint16_t volume, uint8_t num) {
  CalVol[num] = volume;
  if(num>0 && num<=PUMPCOUNT){
    ioDeviceDigitalWriteS(ioExp, num-1, onpump);
    delay (CalVol[num]*kCal[num]);
    ioDeviceDigitalWriteS(ioExp, num-1, !onpump);
  }
}

void i2cPumps::returnCalVol (uint16_t volume, uint8_t num) {


}

void i2cPumps::setPumpScale  (float koef, uint8_t num){
  kCal[num] = koef;
}

float i2cPumps::getPumpScale (uint8_t num){
  return kCal[num];
}

uint8_t i2cPumps::getPumpCount(){
  return uint8_t(PUMPCOUNT);
}


// private:
//   float    kCal[PUMPCOUNT];
//   uint16_t CalVol[PUMPCOUNT];

// void timerMinim::reset() {
//   _timer = millis();
// }