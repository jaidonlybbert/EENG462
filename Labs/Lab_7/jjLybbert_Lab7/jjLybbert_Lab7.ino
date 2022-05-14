#include <MyPrintf.h>

#include<SPI.h>
#include<TimerOne.h>

#define SDI_PIN 51
#define SCK_PIN 52
#define SELECT 8

volatile float data_in[125];
volatile int sample_index = 0;

void setup() {
  Serial.begin(9600);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(SELECT, OUTPUT);
  digitalWrite(SELECT, HIGH);

  // set SPI bit order
  // set SPI data mode
  SPI.beginTransaction(SPISettings(10000, MSBFIRST, SPI_MODE0)); 
  SPI.begin();

  Timer1.initialize(800);

  dacWrite(0);
}

void timerOneISR()
{
  data_in[sample_index] = (float)analogRead(A0);
  sample_index++;
  if (sample_index == 125) Timer1.detachInterrupt();
}

/*
 * Takes 12 bit value and sends to external ADC over SPI
 */
void dacWrite(uint16_t voltage)
{
  voltage *= 1000;
  uint8_t upper = (voltage & 0x0FFF) >> 8;
  upper |= 0x10;
  uint8_t lower = (voltage & 0x00FF);

/*
  Printf("Upper: %x \n", upper);
  Printf("Lower: %x \n", lower);
*/
  
  //set CS low
  digitalWrite(SELECT, LOW);
  // Transfer the value over SPI
  SPI.transfer(upper);
  SPI.transfer(lower);
  digitalWrite(SELECT, HIGH);
}

void loop() {
  sample_index = 0;
  
  dacWrite((uint16_t)3);

  //while(true){;}
  //Serial.print("Voltage read: ");
  //Serial.println((float)analogRead(A1));
  
  Timer1.attachInterrupt(timerOneISR);
  
  while(sample_index < 125) {;}

  dacWrite((uint16_t)0);

  for (int i=0; i<125; i++) {
    Serial.println(data_in[i]);
  }

  delay(500);
}
