#include <MyPrintf.h>
#include <LiquidCrystal.h>
#include <TimerOne.h>
#include <TimerFour.h>

/***************************************************
 * Author: Jaidon Lybbert
 * Date  : 3/22/2022
 * 
 * EENG462 - Final: Kitchen Timer
 * 
 * Description: A kitchen timer using an LCD display
 * 
 *  - The timer has 3 states: 
 *    + STOPPED: the timer is not running, and is not set
 *    + RUNNING: the timer is counting down
 *    + PAUSED:  the countdown is suspended
 *    
 *  - The timer has 2 menus:
 *    + The MAIN menu has 3 options:
 *      - Set:   Changes menu to the SET menu
 *      - Start: sets the timer, and changes timer state to RUNNING
 *      - Pause: toggles timer state between RUNNING and PAUSED
 *    + The SET menu has 2 options:
 *      - MM: allows the user to change the set minutes with the up/down buttons
 *      - SS: allows the user to change the set seconds with the up/down buttons
 * 
 *  - A timer triggered interrupt decrements the
 *    countdown while the timer is running
 * 
 *  - Menu options are organized in a linked list which
 *    can be navigated with the left and right buttons
 *    
 *  - When 'select' is pressed the function associated with
 *    the selected Option is executed
 *   
 ****************************************************/

// LCD module definitions
#define NCOLS 16
#define NROWS 2
#define NBUTTONS 5

// Button definitions
#define RIGHT 0
#define UP 1
#define DOWN 2
#define LEFT 3
#define SELECT 4
#define NONE -1

// IR remote definitions
#define IRMODPWR 53
#define IRMODGND 51
#define IRMODINPUT 49 
#define COMMANDCODE 1
#define REPEATCODE 2
#define ERRCODE 0
#define UPCODE (uint8_t)0x18
#define DOWNCODE (uint8_t)0x52
#define LEFTCODE (uint8_t)0x08
#define RIGHTCODE (uint8_t)0x5a
#define SELCODE (uint8_t)0x1c
#define DATAHIGH !digitalRead(IRMODINPUT)
#define DATALOW digitalRead(IRMODINPUT)

// State definitions
#define STOPPED 0
#define RUNNING 1
#define PAUSED  2
#define COMPLETE 3

// Forward declarations
void printMenu();
void printCountdown();
void print2Digit(int num);

/*
 * The option struct represents available menu options to choose by the user
 * Moving to the right or left with the buttons moves through a linked list of options to change the selected option
 * A function pointer points to the function executed when an option is selected and the 'select' button is pressed
 */
struct Option {
  int    cursorPosition;
  struct Option *next;     // option to the right
  struct Option *previous; // option to the left
  void   (*action)(void);  // pointer to function executed when select is pressed
};

void insertOption(struct Option *option, int cursorPosition, struct Option *next, struct Option *previous, void (*action)(void))
{
  option->cursorPosition = cursorPosition;
  option->next = next;
  option->previous = previous;
  option->action = action;
}

/*
 * The menu struct stores the constant parts of the two possible menus
 * The dynamic parts (countdown, mm, and ss) will be overwritten by functions as needed
 */
struct Menu {
  char TopText[11];
  char BottomText[NCOLS];
};

/*
 * The KitchenTimer struct points to the current menu and selected option, and stores the running countdown
 */
struct KitchenTimer {
  int           runningMinutes;
  int           runningSeconds;
  int           runningTenths;
  int           setMinutes;
  int           setSeconds;
  int           state;
  struct Menu   *ActiveMenu;
  struct Option *ActiveOption;
};

/**************************************************
 * GLOBALS
 *************************************************/
struct Option *Set     = malloc(sizeof (struct Option));
struct Option *Start   = malloc(sizeof (struct Option));
struct Option *Pause   = malloc(sizeof (struct Option));
struct Option *Min     = malloc(sizeof (struct Option));
struct Option *Sec     = malloc(sizeof (struct Option));

struct Menu   *Main    = malloc(sizeof (struct Menu));
struct Menu   *SetTime = malloc(sizeof (struct Menu));

struct KitchenTimer Ktimer { .runningMinutes = 0,
                             .runningSeconds = 0,
                             .runningTenths  = 0,
                             .setMinutes     = 0,
                             .setSeconds     = 0,
                             .state   = STOPPED,
                             .ActiveMenu = Main,
                             .ActiveOption = Set};

LiquidCrystal MyDisplay(8, 9, 4, 5, 6, 7);

/*************************************************
 * TIMER CONFIGURATION
 *************************************************/
/*
 * Decrements countdown when triggered by timer
 */
void timerFourISR()
{
  Ktimer.runningTenths--;

  if (Ktimer.runningTenths < 0) {
    Ktimer.runningTenths = 9;
    Ktimer.runningSeconds--;
  }

  if (Ktimer.runningSeconds < 0) {
    Ktimer.runningSeconds = 59;
    Ktimer.runningMinutes--;
  }

  if (Ktimer.runningMinutes < 0) {
    Ktimer.state = STOPPED;
    Timer4.stop();
    Ktimer.runningTenths = 0;
    Ktimer.runningMinutes = 0;
    Ktimer.runningSeconds = 0;
  }

  printCountdown();
}

void timer1Init()
{
  Timer4.initialize(100000); // Period of 0.1 ms
  Timer4.attachInterrupt(timerFourISR);
}

// initializes timer3 to 64 usec counts
void timer3Init() {
  TCCR3A = 0x00;
  TCCR3B = 0x05; // 16 Mhz / 1024 = 15.625 kHz --> 64 usec per count
  TCCR3C = 0x00;
  TIMSK3 = 0x00; // not using interrupt
  TCNT3 = 0x00; // initial count of 0
}

/**************************************************
 * CALLBACK FUNCTIONS
 *************************************************/
/*
 * Switches menu to 'Set' menu
 */
void setHandler()
{
  Ktimer.ActiveMenu = SetTime;
  Ktimer.ActiveOption = Min;
  printMenu();
  printCountdown();
  MyDisplay.setCursor(Min->cursorPosition, 1);
  print2Digit(Ktimer.setMinutes);
  MyDisplay.setCursor(Sec->cursorPosition, 1);
  print2Digit(Ktimer.setSeconds);
}

/*
 * Starts timer
 */
void startHandler()
{
  Ktimer.state = RUNNING;
  Ktimer.runningMinutes = Ktimer.setMinutes;
  Ktimer.runningSeconds = Ktimer.setSeconds;
  Ktimer.runningTenths = 0;
  printCountdown();
  Timer4.start();
}

/*
 * Pauses and resumes timer
 */
void pauseAndResumeHandler()
{
  if (Ktimer.state == RUNNING){
    Timer4.stop();
    Ktimer.state = PAUSED;
    strcpy(Main->BottomText, "Set Start Resume");
  } else if (Ktimer.state == PAUSED) {
    Ktimer.state = RUNNING;
    Timer4.start();
    strcpy(Main->BottomText, "Set Start Pause ");
  }
  printMenu();
  printCountdown();
}

/*
 * Return to main menu
 */
void returnToMain()
{
  Ktimer.ActiveMenu = Main;
  Ktimer.ActiveOption = Set;
  printMenu();
  printCountdown();
}

/*
 * Handles incrementing minutes and seconds in the 'Set' menu
 */
void upHandler() {
  if (Ktimer.ActiveOption == Min){
    Ktimer.setMinutes++;
    Ktimer.setMinutes = Ktimer.setMinutes % 60;
    print2Digit(Ktimer.setMinutes);
  } else if (Ktimer.ActiveOption == Sec){
    Ktimer.setSeconds++;
    Ktimer.setSeconds = Ktimer.setSeconds % 60;
    print2Digit(Ktimer.setSeconds);
  }
}

/*
 * Handles decrementing minutes and seconds in the 'Set' menu
 */
void downHandler() {
  if (Ktimer.ActiveOption == Min){
    Ktimer.setMinutes--;
    if (Ktimer.setMinutes < 0) Ktimer.setMinutes = 59;
    print2Digit(Ktimer.setMinutes);
  } else if (Ktimer.ActiveOption == Sec){
    Ktimer.setSeconds--;
    if (Ktimer.setSeconds < 0) Ktimer.setSeconds = 59;
    print2Digit(Ktimer.setSeconds);
  }
}

/*******************************************************
 * PRINT FUNCTIONS
 ******************************************************/
/*
 * Prints static portions of menu to the LCD
 */
void printMenu() {
  MyDisplay.setCursor(0,0);
  MyDisplay.print(Ktimer.ActiveMenu->TopText);
  MyDisplay.setCursor(0, 1);
  MyDisplay.print(Ktimer.ActiveMenu->BottomText);
  MyDisplay.setCursor(Ktimer.ActiveOption->cursorPosition, 1);
}

/*
 * Converts and prints 2 integers to LCD
 */
void print2Digit(int num) 
{
    char tempNum[3];
    sprintf(tempNum, "%02d", num);
    for(int i=0; i < 2; i++) MyDisplay.print(tempNum[i]);
}

/*
 * Converts and print countdown to LCD
 */
void printCountdown()
{
  MyDisplay.setCursor(9, 0);
  char countdown[8];
  sprintf(countdown, "%02d:%02d.%1d", Ktimer.runningMinutes, Ktimer.runningSeconds, Ktimer.runningTenths);
  for(int i=0; i<7; i++) MyDisplay.print(countdown[i]);
  MyDisplay.setCursor(Ktimer.ActiveOption->cursorPosition, 1);
}

/*************************************************************
 * BUTTON DETECTION LOGIC
 ************************************************************/
/*
 * Determines button pressed from ADC reading
 */
int findButtonPressed() {
  int ADC_encoding = analogRead(A0);
  int thresholds[5] = {206, 309, 516, 725, 924};

  // search thresholds array for matching button
  for (int i=0; i<NBUTTONS; i++)
  {
    if (ADC_encoding < thresholds[i])
    {
      return i;
    }
  }

  return NONE;
}

/*
 * Chooses action to take based on last button press
 */
void selectAction(int lastbutton)
{
  switch (lastbutton)
  {
    case RIGHT:
      Ktimer.ActiveOption = Ktimer.ActiveOption->next;
      break;
    case UP:
      upHandler();
      break;
    case DOWN:
      downHandler();
      break;
    case LEFT:
      Ktimer.ActiveOption = Ktimer.ActiveOption->previous;
      break;
    case SELECT:
      Ktimer.ActiveOption->action();
      break;
    default:
      break;
  }
  
  MyDisplay.setCursor(Ktimer.ActiveOption->cursorPosition, 1);
}

/***************************************************
 * IR remote
 **************************************************/

void IRremoteInit() {
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

void findIRButtonPressed() {
  uint8_t startCode = waitForStart();
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
}

/***************************************************
 * MAIN
 **************************************************/
void setup() {
  Serial.begin(9600);
  // Build linked list
  insertOption(Set, 0, Start, Pause, setHandler);
  insertOption(Start, 4, Pause, Set, startHandler);
  insertOption(Pause, 10, Set, Start, pauseAndResumeHandler);
  insertOption(Min, 4, Sec, Sec, returnToMain);
  insertOption(Sec, 11, Min, Min, returnToMain);

  // Initialize menus
  strcpy(Main->TopText, "Main       ");
  strcpy(Main->BottomText, "Set Start Pause\0");
  strcpy(SetTime->TopText, "Set Time   ");
  strcpy(SetTime->BottomText, "Min=00 Sec=00  \0");

  // Begin LCD display
  MyDisplay.begin(NCOLS, NROWS);
  MyDisplay.clear();
  MyDisplay.blink();

  // Print menu and countdown
  printMenu();
  printCountdown();

  // Start timer
  timer1Init();
  IRremoteInit();

  delay(500);
}

void loop() {
  /*
  noInterrupts();
  selectAction(findButtonPressed());
  interrupts();
  delay(200);
  */

  while(1) findIRButtonPressed();
}
