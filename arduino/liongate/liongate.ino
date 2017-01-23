#include <SimpleTimer.h>
#include <Servo.h>

// Unlock countdown length (in seconds)
const long unlockCountdownLength = 8 * 60; // 8 minutes (because Nicola said so)

// Unlock countdown step length (in milliseconds)
const long unlockCountdownStepLength = 100;

// Unlock safety step length (in seconds)
const long unlockSafetyStepLength = 10 * 60; // 10 minutes

// Servo
Servo servo;
const int servoPin = 3;

// LED 1: in/out indicator
const int led1RedPin = 11;
const int led1GreenPin = 10;

// LED 2: countdown indicator
const int led2RedPin = 6;
const int led2GreenPin = 5;

// Pushbutton 1: start countdown
const int pushbutton1Pin = 13;

// Pushbutton 2: cat went out
const int pushbutton2Pin = 12;

// Pushbutton 3: cat came in
const int pushbutton3Pin = 8;

// LED power (0 = off, 255 = maximum)o
const int ledPower = 100;

// State
bool isCatIn;
bool pushbutton1Value;
bool pushbutton2Value;
bool pushbutton3Value;
bool isDoorLocked;
SimpleTimer timer;
int unlockCountdownTimerId;
long unlockCountdownElapsedTime;

void setup() {
  Serial.begin(9600);
  
  // LED 1
  pinMode(led1RedPin, OUTPUT);
  pinMode(led1GreenPin, OUTPUT);

  // LED 2
  pinMode(led2RedPin, OUTPUT);
  pinMode(led2GreenPin, OUTPUT);

  // Pushbutton 1
  pinMode(pushbutton1Pin, INPUT);

  // Pushbutton 2
  pinMode(pushbutton2Pin, INPUT);

  // Pushbutton 3
  pinMode(pushbutton3Pin, INPUT);

  // Initial state
  setupUnlockCountdownTimer();
  stopUnlockCountdownAndUnlock(true);
  catCameIn();
  setupUnlockSafetyTimer();

  pushbutton1Value = digitalRead(pushbutton1Pin);
  pushbutton2Value = digitalRead(pushbutton2Pin);
  pushbutton3Value = digitalRead(pushbutton3Pin);
}

void loop() {
  if (checkStartUnlockCountdownAndLock()) {
    startUnlockCountdownAndLock();
  }

  if (checkCatWentOut()) {
    catWentOut();
  }

  if (checkCatCameIn()) {
    catCameIn();
  }

  timer.run();
}

void setupUnlockCountdownTimer() {
  unlockCountdownTimerId = -1;
  unlockCountdownElapsedTime = 0L;
}

bool checkStartUnlockCountdownAndLock() {
  return checkPushButtonPressed(pushbutton1Pin, &pushbutton1Value);
}

void unlockCountdownStep() {
  unlockCountdownStep(false);
}

void unlockCountdownStep(bool force) {
  if (force || isUnlockCountdownActive()) {
    unlockCountdownElapsedTime += unlockCountdownStepLength;
    if (unlockCountdownElapsedTime >= unlockCountdownLength * 1000L) {
      stopUnlockCountdownAndUnlock(true);
      return;
    }

    int progress = (int) ((unlockCountdownElapsedTime * ledPower) / (unlockCountdownLength * 1000.0d));
    analogWrite(led2RedPin, progress);
    analogWrite(led2GreenPin, ledPower - progress);
  }
}

void startUnlockCountdownAndLock() {
  startUnlockCountdown();
  lockDoor(false);
}

void startUnlockCountdown() {
  stopUnlockCountdown();
  unlockCountdownStep(true);
  unlockCountdownTimerId = timer.setInterval(unlockCountdownStepLength, unlockCountdownStep);
}

void stopUnlockCountdown() {
  if (isUnlockCountdownActive()) {
    timer.deleteTimer(unlockCountdownTimerId);
    unlockCountdownTimerId = -1;
  }

  unlockCountdownElapsedTime = 0L;
  analogWrite(led2RedPin, 0);
  analogWrite(led2GreenPin, 0);
}

void stopUnlockCountdownAndUnlock(boolean forceUnlock) {
  stopUnlockCountdown();
  unlockDoor(forceUnlock);
}

bool isUnlockCountdownActive() {
  return unlockCountdownTimerId >= 0;
}

void unlockSafetyStep() {
  // Attach & detach
  // Makes sure the servo goes in the position it's supposed to be
  // In case the last time it got moved it was stuck or something
  if(!servo.attached()) {
    servo.attach(servoPin);
    delay(500);
    servo.detach();
  }
}

void setupUnlockSafetyTimer() {
  timer.setInterval(unlockSafetyStepLength * 1000L, unlockSafetyStep);
}

bool checkCatWentOut() {
  return isCatIn && checkPushButtonPressed(pushbutton2Pin, &pushbutton2Value);
}

void catWentOut() {
  analogWrite(led1RedPin, 255);
  analogWrite(led1GreenPin, 0);
  isCatIn = false;
}

bool checkCatCameIn() {
  return !isCatIn && checkPushButtonPressed(pushbutton3Pin, &pushbutton3Value);
}

void catCameIn() {
  analogWrite(led1RedPin, 0);
  analogWrite(led1GreenPin, 255);
  isCatIn = true;
}

void unlockDoor(bool force) {
  if (force || isDoorLocked) {
    isDoorLocked = false;
    servo.attach(servoPin);
    servo.write(0);
    delay(500);
    servo.detach();
  }
}

void lockDoor(bool force) {
  if (force || !isDoorLocked) {
    isDoorLocked = true;
    servo.attach(servoPin);
    servo.write(180);
    delay(500);
    servo.detach();
  }
}

bool checkPushButtonPressed(int switchPin, bool * last) {
  bool current = digitalRead(switchPin);
  if (current == HIGH) {
    // Debounce
    delay(5);
    current = digitalRead(switchPin);
  }

  // Button is considered pressed on release
  bool pushButtonPressed = *last == HIGH && current == LOW;

  *last = current;

  return pushButtonPressed;
}
