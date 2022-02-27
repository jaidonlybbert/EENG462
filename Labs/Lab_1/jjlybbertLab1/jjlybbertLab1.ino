/*
 * EENG 462 Lab 1
 * Jaidon Lybbert
 * 1-17-2022
 */


/*
 * == Button Encoding ==
 * Button|Voltage|Predicted|Tested
 * Right : 0.00V -> 0       0
 *                  104     103
 * Up    : 1.02V -> 208     206
 *                  309     309
 * Down  : 2.00V -> 409     407
 *                  517     516
 * Left  : 3.05V -> 624     625
 *                  724     725
 * Select: 4.02V -> 823     824
 *                  923     924
 * None  : 5.00V -> 1023    1023
 */

#include <LiquidCrystal.h>
#define NCOLS 16
#define NROWS 2
#define NBUTTONS 5

LiquidCrystal MyDisplay(8, 9, 4, 5, 6, 7);

int findButtonPressed() {
  int ADC_encoding = analogRead(A0);
  int thresholds[5] = {206, 309, 516, 725, 924};

  /* 
   *  Button pressed represented by a number, as follows
   *  Right => 0
   *  Up    => 1
   *  Down  => 2
   *  Left  => 3
   *  Select=> 4
   *  None  => -1
   */

  // search thresholds array for matching button
  for (int i=0; i<NBUTTONS; i++)
  {
    if (ADC_encoding < thresholds[i])
    {
      return i;
    }
  }

  return -1;
}

void setup() {
  MyDisplay.begin(NCOLS, NROWS);
  MyDisplay.clear();
  MyDisplay.print("     Hello      ADC Key Testing");
  MyDisplay.setCursor(5, 1);
  MyDisplay.print("World!");

  delay(500);

  for(int i=0; i < NCOLS; i++)
  {
    MyDisplay.scrollDisplayLeft();
    delay(500);
  }
}

void loop() {
  int lastbutton = findButtonPressed();

  // clear display only when a button is pressed
  // when no button is pressed, the display is not changed
  if (lastbutton != -1) MyDisplay.clear();
  MyDisplay.setCursor(0, 1);

  switch (lastbutton)
  {
    case 0:
      MyDisplay.print("Right");
      break;
    case 1:
      MyDisplay.print("Up");
      break;
    case 2:
      MyDisplay.print("Down");
      break;
    case 3:
      MyDisplay.print("Left");
      break;
    case 4:
      MyDisplay.print("Select");
      break;
    default:
      break;
  }

  delay(25);
}
