#ifndef SERVO_MANAGER_HPP
#define SERVO_MANAGER_HPP

// Inclusion de la librairie ESP32Servo (compatible Arduino Servo)
#include <ESP32Servo.h> 
#include "config.hpp"

// Déclaration externe de l'objet Servo
extern Servo monServo;

void initialiser_servo();

void ouvrirPorte();

#endif