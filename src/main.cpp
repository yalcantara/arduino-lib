#include <Arduino.h>
#include <ctime>
#include <Servo.h>

#include <ylib/core/core.h>

#include "ylib/railroad/Turnout.h"

using namespace ylib::core;
using namespace ylib::railroad;

Timer turnoutTimer(50);

Turnout t1("A1", 4, RIGHT, 94, 73);
Turnout t2("A2", 5, RIGHT, 93, 72);

bool setUpSuccess = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting up...");
  Turnout::SLOW_MOVE = true;

  setUpSuccess =
    t1.setup() &&
    t2.setup();
}

void loop() {
  if (!setUpSuccess) {
    Serial.println("The setup() failed");
    delay(3000);
    return;
  }


  //t1.straight();
  if (turnoutTimer.next()) {
    t1.loop();
    t2.loop();
  }


  if (!t1.isInProgress()) {
    delay(4000);
    t1.invert();
  }
/*
  if (!t2.isInProgress()) {
    delay(4000);
    t2.invert();
  }*/

  delay(1);
}
