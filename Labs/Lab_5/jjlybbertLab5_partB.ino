#include <ARTK.h>

#define INPUT_SW 19
#define BUTTON_DOWN !digitalRead(INPUT_SW)
#define BUTTON_UP digitalRead(INPUT_SW)

SEMAPHORE pressed;
SEMAPHORE processed;
void poll(), process_press(), background_process();

void Setup() {
  ARTK_SetOptions(1,-1);             // Set to large memory model

  pinMode(INPUT_SW, INPUT_PULLUP);

  pressed = ARTK_CreateSema(0);
  processed = ARTK_CreateSema(0);

  ARTK_CreateTask(poll, 3);
  ARTK_CreateTask(process_press, 2);
  ARTK_CreateTask(background_process, 1);
}

/* switch monitoring task */
void poll() {
  bool button_was_released = 0;
  while(1)
  {
    if (BUTTON_UP) {
      button_was_released = 1;
    } else {                            // button down
      if (button_was_released) {        // if button was released since last trigger, must be a falling edge
        Printf(".");
        ARTK_SignalSema(pressed);       // signal semaphore for processing task
        ARTK_WaitSema(processed);        // reset, so triggers don't repeat until released and pressed again
        button_was_released = 0;
      }
    }
    ARTK_Sleep(20);
  }
}

/* processes switch presses */
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

/* background processes */
void background_process() {
  volatile long randint;
  while(1) {
    randint = random(33, 128); // range 33-127
    Printf(F("%c"), (char)randint);    // print as char
    delay(1000);               // delay for 1 s
  }
}
