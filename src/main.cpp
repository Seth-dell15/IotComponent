#include <Arduino.h>
#include "app_logic.hpp" 

void setup() {
    AppLogic::initialize();
}

void loop() {
    AppLogic::execute();
}