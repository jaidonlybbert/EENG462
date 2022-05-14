#include <ARTK.h>

#define INPUT_SW 19
#define BUTTON_DOWN_QUERY !digitalRead(INPUT_SW)
#define BUTTON_UP_QUERY digitalRead(INPUT_SW)
#define B_UNPRESSED false
#define B_PRESSED true

SEMAPHORE triggered;
SEMAPHORE pressed;
SEMAPHORE processed;
SEMAPHORE busMutex;

void validateTrigger(), process_press(), TaskL(), switchTriggeredHandler();

int timestamp = millis();


/*
 * Setup tasks, semaphores, interrupts
 */
void Setup() {
  ARTK_SetOptions(1,-1);             // Set to large memory model

  pinMode(INPUT_SW, INPUT_PULLUP);

  triggered = ARTK_CreateSema(0);
  pressed = ARTK_CreateSema(0);
  processed = ARTK_CreateSema(0);
  busMutex = ARTK_CreateSema(1);

  attachInterrupt(digitalPinToInterrupt(INPUT_SW), switchTriggeredHandler, FALLING);
  
  ARTK_CreateTask(validateTrigger, 4);
  ARTK_CreateTask(TaskL, 1);
  ARTK_CreateTask(TaskM, 2);
  ARTK_CreateTask(TaskH, 3);
  
  WatchdogInit(5);
}


/*
 * Switch trigger interrupt service routine
 */
void switchTriggeredHandler() 
{
  if ((millis() - timestamp) > 20) {
      timestamp = millis();
      ARTK_SignalSema(triggered);
  }
}


/* 
 *  Checks switch state 18 ms after trigger registered, and signals press if switch is down
 *  Waits for press to be processed before listening for more trigger events
*/
void validateTrigger() 
{
  while(1) {
    ARTK_WaitSema(triggered);
    ARTK_Sleep(18);

    if (BUTTON_DOWN_QUERY) {
      Printf(".");
      ARTK_SignalSema(pressed);
      ARTK_WaitSema(processed);
    }
  }
}


/* 
 *  Lowest priority task using bus
*/
void TaskL() {
  while(1) {
    Printf("\nLOW waiting on bus");    // print as char
	ARTK_WaitSema(busMutex);
	Printf("\nLOW has the bus");
    UseWatchedBus(2);
	Printf("\nLOW releasing bus");
	ARTK_SignalSema(busMutex);
  }
}

/*
 * Medium priority task
*/
void TaskM() {
  while(1) {
      ARTK_WaitSema(pressed);
      Printf("\n* MEDIUM is computing *");
	  delay(7000);
	  Printf("\nDone computing");
      ARTK_SignalSema(processed);
  }
}

/*
 * High priority task using bus
*/
void TaskH() {
  while(1) {
    Printf("\nHIGH waiting on bus");    // print as char
	ARTK_WaitSema(busMutex);
	Printf("\nHIGH has the bus");
    UseWatchedBus(2);
	Printf("\nHIGH releasing bus");
	ARTK_SignalSema(busMutex);
	ARTK_Sleep(1000);
  }
}