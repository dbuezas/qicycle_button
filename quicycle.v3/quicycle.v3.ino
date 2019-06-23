// All pins are capable of Digital output, though P5 is 3 V at HIGH instead of 5 V
//    pinMode(0, OUTPUT); //0 is P0, 1 is P1, 2 is P2, etc. - unlike the analog inputs, for digital outputs the pin number matches.

//#define DEBUG // this will flash the led

#include "./EEPROM/EEPROM.h"

#define BUTTON_PIN 0
#define LED_PIN 1
#define THROTTLE_PIN 2

#define DBL_CLICK_MS 300
#define CLICK_MS 200

#define ON_TIME_MS 4500
#define OFF_TIME_MS 20

#define KILL_CLICKS 5
#define CRUISE_CLICKS 2

#define KILL_SWITCH_EEPROM_ADDR 1

bool killSwitch = true;
bool isCruising = false;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(THROTTLE_PIN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, true); // pull UP P0
  toggleMotor(false);
  blink(5);
  killSwitch = EEPROM.read(KILL_SWITCH_EEPROM_ADDR);
}

bool lastMotorIsOn = true;
void toggleMotor(bool isOn) {
  if (isOn != lastMotorIsOn) {
      lastMotorIsOn = isOn;
    #ifdef DEBUG
      digitalWrite(LED_PIN, isOn);
    #endif
    digitalWrite(THROTTLE_PIN, isOn);  
  }
}

void blink(int times) {
  #ifdef DEBUG
    for (int i = 0; i< times; i++) {
      digitalWrite(LED_PIN, true);
      delay(50);
      digitalWrite(LED_PIN, false);
      delay(50);
    }
  #endif
}

long lastBtnSeemsUpMs = 0;
long lastBtnSeemsDownMs = 0;
bool lastBtnSeemsDown = false;

bool getIsBtnDownDebounced() {
  // non-blocking debounce, important to keep good timings
  bool btnSeemsDown = !digitalRead(BUTTON_PIN); 
  if (btnSeemsDown) {
    lastBtnSeemsDownMs = millis();
  } else {
    lastBtnSeemsUpMs = millis();
  }
  long deltaMs = abs(lastBtnSeemsDownMs - lastBtnSeemsUpMs);
  if (deltaMs > 10) {
    // no changes for 25ms, reading is confirmed.
    lastBtnSeemsDown = btnSeemsDown;
  };
  return lastBtnSeemsDown;
}

bool wasBtnDown = false;
long lastBtnToggleMs = 0;
long lastClickMs = 0;
int clickCount = 0;

void loop() {
  long nowMs = millis();
  bool isBtnDown = getIsBtnDownDebounced();    
  
  if (isBtnDown) {
    isCruising = false;
  } else { // isBtnDown === false
    long msSinceLastBtnDown = nowMs - lastBtnToggleMs;
    bool isClick = wasBtnDown && (msSinceLastBtnDown < CLICK_MS);
    if (isClick) {
      lastClickMs = nowMs;
      clickCount++;
    } else {
      long msSinceLastClick = nowMs - lastClickMs;
      bool canConfirmOldClicks = msSinceLastClick > DBL_CLICK_MS;
  
      if (canConfirmOldClicks && clickCount > 0){
        if (clickCount == KILL_CLICKS) {
          killSwitch = !killSwitch;
          blink(killSwitch ? 2 : 5);
          EEPROM.write(KILL_SWITCH_EEPROM_ADDR, killSwitch);
        }
        if (clickCount == CRUISE_CLICKS) {
          isCruising = true;
        }
        clickCount = 0;
      }
    }
  }
  if (wasBtnDown != isBtnDown) lastBtnToggleMs = nowMs;

  wasBtnDown = isBtnDown;

  bool shouldPause = ((nowMs -lastBtnToggleMs) % ON_TIME_MS) < OFF_TIME_MS;
  bool wantsToAccelerate = isBtnDown || isCruising;
  
  toggleMotor(!killSwitch && wantsToAccelerate && !shouldPause);    
}
