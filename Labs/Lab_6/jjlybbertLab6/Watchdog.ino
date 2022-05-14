// Timer 3 creates a weird problem if Printf is used 
// while manipulating TCNT3, but Serial.print works fine
#include <TimerFour.h>
#include <stdint.h>
#include <avr/wdt.h>
#define SW_INT_PIN 2

// A hardware watchdog reset on the Mega will prevent further code
// downloads until power is cycled. Another strategy is to use the 
// address 0 reset but that also does not work on the Mega. Define 
// this to just go into a dead loop instead of trying to reset 
// the processor
#define __NO_RESET

boolean __watchdog_running = false ;
int __seconds = 0 ;
int __terminal_cnt = 0 ;

void WatchdogInit(long sec) {
  pinMode(SW_INT_PIN, OUTPUT) ;
  digitalWrite(SW_INT_PIN, LOW) ;
  attachInterrupt(digitalPinToInterrupt(SW_INT_PIN), swIntISR, RISING) ;

  __watchdog_running = true ;
  __seconds = sec ;
  __terminal_cnt = 0 ;
  
  Timer4.initialize(1000000) ;
  Timer4.attachInterrupt(WatchdogISR) ;
}

void reboot() {
  #ifndef __NO_RESET
     wdt_disable();
     wdt_enable(WDTO_15MS);
  #endif
  // ARTK_TerminateMultitasking() ;
  // wait for the timeout
  while (1) {}
}

void WatchdogISR() {
   unsigned long now = millis()/1000;
   if (now<1) return ;

   __terminal_cnt++ ;
   if (__terminal_cnt>=__seconds) {
      Printf(F("\nWATCHDOG EXPIRED - RESET\n")) ;
      reboot() ;
   }
}

void swIntISR() {
   digitalWrite(SW_INT_PIN, LOW) ; 
   TCNT4 = 1 ;
   __terminal_cnt = 0 ;
   // Printf("SW Interrupt\n") ;
}

void UseWatchedBus(int sec) {
   // simulate bus activity every tenth of a second by causing 
   // a software interrupt (will stop happening while Task is swapped out)
   int tenths = sec*10 ;
   for (int i=0 ; i<tenths ; i++) {
      if (__watchdog_running) digitalWrite(SW_INT_PIN, HIGH) ;
      Printf(".") ;
      delay(100) ;
   }
}

void fireSwInt() {
   digitalWrite(SW_INT_PIN, HIGH) ;  
}
