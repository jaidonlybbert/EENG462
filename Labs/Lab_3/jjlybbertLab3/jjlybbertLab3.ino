/*
   Jaidon Lybbert
   EENG 462
   Lab 3 - Settling Time, Nonlinear Calibration

   Led pwm - pin 9
   Analog in - pin A0

   Regression model:
   a = 0.02797
   b = 0.01007

*/

#define LEDPIN 9
#define ANALOGIN A0
#define A 0.02797
#define B 0.01007

// Calculated the value settled to after 1 second
float findMaxValue() {
  // Turn LED on with 100% duty cycle, and wait for settling
  analogWrite(LEDPIN, 255);
  delay(1000);

  // Average 20 data points after settling to find max value
  float sum = 0;

  for(int i=0; i < 20; i++)
  {
    sum += (float)analogRead(ANALOGIN);
  }
  
  float max_value = sum/20;
  
  analogWrite(LEDPIN, 0);

  delay(1000);
  
  return max_value;
}

// Calculate the lower value with LED off
float findMinValue() {
  // Turn LED off
  analogWrite(LEDPIN, 0);
  delay(1000);

  // Average 20 data points after falling to find min value
  float sum = 0;

  for (int i=0; i<20; i++)
  {
    sum += (float)analogRead(ANALOGIN);
  }

  float min_val = sum/20;

  return min_val;
}

// Calculate settling time in ms
unsigned long findSettlingTime() {
  float max_value = findMaxValue();
  float current_value = 0;

  analogWrite(LEDPIN, 255);

  // Measure time in ms from turning on to 99% max value
  unsigned long start_ms = millis();
  while(current_value < (0.99*max_value))
  {
    current_value = (float)analogRead(ANALOGIN);
  }
  unsigned long finish_ms = millis();

  unsigned long settle_time = finish_ms - start_ms;

  for (int i = 0; i < 5; i++)
  {
    Serial.print(settle_time);
    Serial.print(",");
    Serial.println(settle_time);
  }


  analogWrite(LEDPIN, 0);
  delay(settle_time);

  return settle_time;
}


/*
 * Plots sharktooth wave generated from turning led on and off: not part of assignment
 */
void plotChargeDischarge() {
  float maxVal = findMaxValue();
  float minVal = findMinValue();

  float currentVal = 0;

  analogWrite(LEDPIN, 255);

  while (currentVal < (0.99*maxVal))
  {
    currentVal = (float)analogRead(ANALOGIN);
    Serial.println(currentVal);
  }

  analogWrite(LEDPIN, 0);

  while (currentVal > (0.01*(maxVal-minVal))+minVal)
  {
    currentVal = (float)analogRead(ANALOGIN);
    Serial.println(currentVal);
  }
}


/*
 * Plots response of photoresistor circuit
 * 
 * Graph shows voltage vs. led pwm duty cycle as a value 0-255
 */
float* plotResponse(unsigned long settle_time_ms, float* dataIn) {
  for (int i = 0; i < 256; i++)
  {
    analogWrite(LEDPIN, i);
    delay(settle_time_ms);
    dataIn[i] = (float)analogRead(ANALOGIN);
  }
  
  for (int i = 0; i < 256; i++)
  {
    Serial.print(dataIn[i]);
    Serial.print(",");
    Serial.println(i);
  }
}

/*
 * Plots photo-resistor response compared to exponential mapping
 */
void plotRegression(float* measuredData) {
  for (int i = 0; i < 256; i++)
  {
    Serial.print(A*exp((float)B*measuredData[i]));
    Serial.print(",");
    Serial.println(i);
  }
}


/*
 * Main setup function
 */
void setup() {
  Serial.begin(9600);
  TCCR2B = TCCR2B & B11111000 | B00000001 ; // PWM period

  // Calculate settling time
  unsigned long settle_time_ms = findSettlingTime();

  float dataIn[256];

  plotResponse(settle_time_ms, dataIn);

  plotRegression(dataIn);
}

void loop() {}
