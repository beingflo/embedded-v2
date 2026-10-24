#include "TM1637.h"

const int CLK = D3;
const int DIO = D2;

TM1637 tm(CLK, DIO);

int number = 9999;

void setup()
{
  tm.begin();
  tm.setBrightness(2);
}

void loop()
{
  tm.display(number);

  number -= 1;

  delay(10);
}
