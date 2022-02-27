/*
 * Jaidon Lybbert
 * 1/28/22
 * EENG462 - Lab 2
 * 
 * Task 1: Estimate is 8.5 kHz sample rate
 *                  or 118 us sample period
 *                          
 */

 

void setup() {
  Serial.begin(9600);
}

void clear_output() {
  analogWrite(9, 0);
  delay(50);
}

void loop() {
  // default timer period
  TCCR2B = TCCR2B & B11111000 | B00000111 ; // slow 
  
  // wait for key pressed
  while(Serial.available()==0)
  {}

  // Check key and respond accordingly
  char x = Serial.read();
  int level = 0;
  int analog_in = A1;
  float task1_read[500];
  switch (x)
  {
    case '5'  :  
      level = 128;
      analog_in = A1;
      break;
    case '2'  :
      level = 64;
      analog_in = A1;
      break;
    case '7'  :
      level = 192;
      analog_in = A1;
      break;
    case 's'  :
      TCCR2B = TCCR2B & B11111000 | B00000011 ; // slow 
      analog_in = A0;
      level = 128;
      clear_output();
      break;
    default   :
      return;
  }

  analogWrite(9, level); //pin 9 val 128
  for(int i=0; i<500;i++) 
  {
    task1_read[i] = (float)analogRead(analog_in); // read from analog input
  }

  for (int i=0; i<500; i++)
  {
    Serial.println(task1_read[i]);
  }
  
}
