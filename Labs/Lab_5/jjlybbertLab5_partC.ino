#include <ARTK.h>

#define INPUT_SW 19
#define BUTTON_DOWN_QUERY !digitalRead(INPUT_SW)
#define BUTTON_UP_QUERY digitalRead(INPUT_SW)
#define B_UNPRESSED false
#define B_PRESSED true

SEMAPHORE triggered;
SEMAPHORE pressed;
SEMAPHORE processed;

void validateTrigger(), process_press(), background_process(), switchTriggeredHandler();

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

  attachInterrupt(digitalPinToInterrupt(INPUT_SW), switchTriggeredHandler, FALLING);
  
  ARTK_CreateTask(validateTrigger, 3);
  ARTK_CreateTask(process_press, 2);
  ARTK_CreateTask(background_process, 1);
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
 *  Processes switch presses
*/
void process_press() {
  static int counter = 0;
  while(1) {
      ARTK_WaitSema(pressed);                // Wait for button press
      counter++;
      delay(1000);
      Printf(F("\nProcessed %d presses\n"), counter);
      if (counter>=10) {
        ARTK_TerminateMultitasking();
      }
      ARTK_SignalSema(processed);
  }
}


/* 
 *  Background process prints random characters
*/
void background_process() {
  volatile long randint;
  while(1) {
    randint = random(33, 128); // range 33-127
    Printf(F("%c"), (char)randint);    // print as char
    delay(1000);               // delay for 1 s
  }
}
