/*
   Jaidon Lybbert
   EENG 462
   Lab 4 - IR Remote Decoding

   IR Sensor is active low
*/

#include <MyPrintf.h>

#define IRMODPWR 53
#define IRMODGND 51
#define IRMODINPUT 49 
#define COMMANDCODE 1
#define REPEATCODE 2
#define ERRCODE 0
#define DATAHIGH !digitalRead(IRMODINPUT)
#define DATALOW digitalRead(IRMODINPUT)

// initializes timer3 to 64 usec counts
void timer3Init() {
  TCCR3A = 0x00;
  TCCR3B = 0x05; // 16 Mhz / 1024 = 15.625 kHz --> 64 usec per count
  TCCR3C = 0x00;
  TIMSK3 = 0x00; // not using interrupt
  TCNT3 = 0x00; // initial count of 0
}

void setup() {
  Serial.begin(9600);
  
  pinMode(IRMODPWR, OUTPUT);
  pinMode(IRMODGND, OUTPUT);
  pinMode(IRMODINPUT, INPUT);

  digitalWrite(IRMODPWR, HIGH);
  digitalWrite(IRMODGND, LOW);

  timer3Init();
}

uint8_t waitForStart() {
  int commandStartSeq[2] = {9000, 4500};
  int repeatStartSeq[2] = {9000, 2250};
  int duration1_us = 0;
  int duration2_us = 0;
  uint16_t count = 0x00;
  int retval;
  
  while(DATALOW){;}

  TCNT3 = 0x00;

  while(DATAHIGH){;}

  count = TCNT3;
  TCNT3 = 0x00;
  duration1_us = 64*count; // time in us

  if ((duration1_us < commandStartSeq[0]*1.2) 
       and (duration1_us > commandStartSeq[0]*0.8))
  {
    while(DATALOW){;}

    count = TCNT3;
    TCNT3 = 0x00;
    duration2_us = 64*count;
    
    if ((duration2_us < commandStartSeq[1]*1.2) 
         and (duration2_us > commandStartSeq[1]*0.8))
    {
      retval = COMMANDCODE;
    } 
    else if ((duration2_us < repeatStartSeq[1]*1.2) 
                and (duration2_us > repeatStartSeq[1]*0.8)) 
    {
      retval = REPEATCODE;
    }
    
  } 
  else 
  {
    retval = ERRCODE;
  }

/*
  Serial.print("Duration 1: ");
  Serial.println(duration1_us);
  Serial.print("Duration 2: ");
  Serial.println(duration2_us);
*/
  return retval;
}

uint8_t readBit() {
  int bitStart_us = 563;
  int bitLow_us = 563;
  int bitHigh_us = 1688;
  uint16_t count = 0x00;
  int duration_us = 0;

  while(DATAHIGH){;}

  count = TCNT3;
  TCNT3 = 0x00;
  duration_us = 64*count;

  if ((duration_us < bitStart_us * 1.2) 
       and (duration_us > bitStart_us * 0.8))
  {
    while(DATALOW){;}

    count = TCNT3;
    TCNT3 = 0x00;
    duration_us = 64*count;

    if ((duration_us < bitLow_us * 1.2)
         and (duration_us > bitLow_us * 0.8))
    {
      return 0; // Bit low
    }
    else if ((duration_us < bitHigh_us * 1.2)
              and (duration_us > bitHigh_us * 0.8))
    {
      return 1; // Bit high
    }
    else
    {
      return 2; // Bit error
    }
  }
  else
  {
    return 2; // Bit error
  }
}

uint8_t readByte() {
  uint8_t byte_recieved;

  for(int i=0; i<8; i++) {
    byte_recieved = (byte_recieved >> 1) | (readBit() << 7);
  }

  return byte_recieved;
}

void loop() {
  uint8_t startCode = waitForStart();
  // Static so they can be used in the future when repeat is recieved
  static uint8_t address; 
  static uint8_t address_inverse;
  static uint8_t command;
  static uint8_t command_inverse;
  
  if (startCode == COMMANDCODE)
  {
    address = readByte();
    address_inverse = readByte();
    command = readByte();
    command_inverse = readByte();

    if ((address | address_inverse) != 0xFF)
    {
      Printf("Bad Address Check\n");
    }
    else
    {
      Printf("Address: 0x%2x ", address);
    }
    
    if ((command | command_inverse) != 0xFF)
    {
      Printf("Bad Command Check\n");
    }
    else
    {
      Printf("Command: 0x%2x \n\n", command);
    }
    
  }
  else if (startCode == REPEATCODE)
  {
    Printf("REPEAT\n\n");
  }
  else if (startCode == ERRCODE)
  {
    Printf("Start bit error\n\n");
  }

/*
  if ((startCode == COMMANDCODE) or (startCode == REPEATCODE))
  {
    Serial.print("Start code: ");
    Serial.println(startCode, HEX);
    Serial.print("Address: ");
    Serial.println(address, HEX);
    Serial.print("Address inv: ");
    Serial.println(address_inverse, HEX);
    Serial.print("Command: ");
    Serial.println(command, HEX);
    Serial.print("Command inv: ");
    Serial.println(command_inverse, HEX);
    Serial.println("\n");
  }
*/
}
