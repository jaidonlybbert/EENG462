/*
 * PID control of an RLC circuit
 * 
 * EENG462 - W22
 * Jaidon Lybbert
 */

#include <MyPrintf.h>
#include<SPI.h>
#include<TimerOne.h>

// I/O map
#define SDI_PIN 51
#define SCK_PIN 52
#define SELECT 8
// PID constants
#define KP 8.5
#define KI 650.0
#define KD 0.06
#define SAMPLE_T 400 // microseconds

volatile float open_loop_output[125];
volatile float closed_loop_output[125];
volatile float pid_output[125];
volatile int sample_index = 0;

void setup() {
  Serial.begin(9600);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(SELECT, OUTPUT);
  digitalWrite(SELECT, HIGH);

  // set SPI bit order
  // set SPI data mode
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); 
  SPI.begin();

  Timer1.initialize(SAMPLE_T);

  dacWrite(0);
}

void OpenLoopISR()
{
  open_loop_output[sample_index] = (float)analogRead(A0)*5/1023;
  sample_index++;
  if (sample_index == 125) Timer1.detachInterrupt();
}

/*
 * Calculates PID output given error
 */
float PID(float error, bool reset) {
  static float integrated_error = 0;
  static float previous_error = 0;

  if (reset) {
    integrated_error = 0;
    previous_error = 0;
    return 0;
  }

  float derivative_error = (error-previous_error)/(SAMPLE_T*0.000001);
  float output = (error * KP) + (integrated_error * KI) + (KD * derivative_error);

  previous_error = error;
  integrated_error += (error*SAMPLE_T*0.000001);

  return output;
}

/*
 * Applies PID correction to signal
 */
void PID_ISR()
{
  // read voltage
  closed_loop_output[sample_index] = (float)analogRead(A0)*5/1023;
  // calc error
  float err = 3.0 - closed_loop_output[sample_index];
  // calc pid output
  float pid_out = PID(err, false);

  if (pid_out < 0) {
    pid_out = 0;
  } else if (pid_out > 4.095) {
    pid_out = 4.095;
  }

  //pid_output[sample_index] = pid_out;

  dacWrite(pid_out);
  
  sample_index++;
  if (sample_index == 125) {
    err = PID(0, true);
    Timer1.detachInterrupt();
  }
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
  // Record open loop response
  sample_index = 0;
  dacWrite((uint16_t)3);
  Timer1.attachInterrupt(OpenLoopISR);
  while(sample_index < 125) {;}
  dacWrite((uint16_t)0);
  delay(500);

  /*
   * My code doesn't run without this println statement here
   * Very confusing.
   */
  Serial.println(0);
  
  // Record closed-loop response
  sample_index = 0;
  dacWrite((uint16_t)3);
  Timer1.attachInterrupt(PID_ISR);
  while(sample_index < 125) {;}
  dacWrite((uint16_t)0);
  delay(500);

  for (int i=0; i<125; i++) {
    Serial.print("Open-loop:");
    Serial.print(open_loop_output[i]);
    Serial.print(",");
    Serial.print("Closed-loop:");
    Serial.println(closed_loop_output[i]);
    //Serial.print(",");
    //Serial.print("PID-out:");
    //Serial.println(pid_output[i]);
  }  
}
