#include "i2cPumps.h"
#ifdef PHTDSCONTROL
i2cPumps::i2cPumps(boolean revers)
{
  onpump = revers;
  for(int i = 0; i <= PUMPCOUNT-1; i++ ){
    scaleCal[i]=10;
  }
}

boolean   i2cPumps::pourVol (uint16_t volume, uint8_t num){
  boolean result = false;
  Serial.print(num);
  Serial.print(" ");
  Serial.print(volume);
  Serial.print(" "); 
  Serial.println(scaleCal[num-1]);

  if(num >= 1 && num <= PUMPCOUNT){
    mcp.digitalWrite(num-1, !pumps.getinit());
    mcp.digitalWrite(num+7, pumps.getinit());
    delay (volume*scaleCal[num-1]);
    mcp.digitalWrite(num-1, pumps.getinit());
    mcp.digitalWrite(num+7, pumps.getinit());

//   mcp.digitalWrite(num-1, onpump);
//    mcp.digitalWrite(num-1, !onpump);
    result = true;
  }
  return result;
}

void i2cPumps::pourCalVol (uint16_t volume, uint8_t num) {
  CalVol[num-1] = volume;
  if(num>0 && num<=PUMPCOUNT){
    mcp.digitalWrite(num-1, !pumps.getinit());
    mcp.digitalWrite(num+7, pumps.getinit());
    delay (CalVol[num-1]*scaleCal[num-1]);
    mcp.digitalWrite(num-1, pumps.getinit());
    mcp.digitalWrite(num+7, pumps.getinit());
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

boolean  i2cPumps::getinit (){
  return (onpump);
}
