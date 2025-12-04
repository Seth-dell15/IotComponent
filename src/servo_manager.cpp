#include "servo_manager.hpp"
#include "config.hpp"

// Définition de l'objet Servo
Servo monServo; 

/**
 * Initialise le servomoteur à rotation continue (Attachement à la broche 26).
 */
void initialiser_servo() {
    // Allocation du timer PWM (essentiel pour la stabilité sur ESP32)
    ESP32PWM::allocateTimer(0);
    monServo.attach(BROCHE_SERVO); 
    delay(100); 
    monServo.write(SERVO_STOP_VALUE); 
    Serial.printf("Servo attache au GPIO %d. Position initiale: %d (ARRET).\n", BROCHE_SERVO, SERVO_STOP_VALUE);
}

/**
 * Deplace le servomoteur CONTINU en activant une rotation puis en s'arrêtant.
 */
void ouvrirPorte(){
    Serial.println("Ouverture de la porte...");
    
    // 1. COMMANDE D'OUVERTURE (Rotation)
    monServo.write(SERVO_OUVERTURE_VITESSE);
    Serial.printf("Servo: Ouverture VITESSE %d (duree %dms)\n", SERVO_OUVERTURE_VITESSE, DUREE_MOUVEMENT_MS);
    delay(DUREE_MOUVEMENT_MS); 
    
    // 2. ARRET du servo
    monServo.write(SERVO_STOP_VALUE); 
    Serial.printf("Servo: ARRET intermediaire %d\n", SERVO_STOP_VALUE);
    
    // Maintient la position ouverte
    delay(2500); 

    // 3. COMMANDE DE FERMETURE (Rotation inverse)
    monServo.write(SERVO_FERMETURE_VITESSE);
    Serial.printf("Servo: Fermeture VITESSE %d (duree %dms)\n", SERVO_FERMETURE_VITESSE, DUREE_MOUVEMENT_MS);
    delay(DUREE_MOUVEMENT_MS);
    
    // 4. ARRET FINAL du servo
    monServo.write(SERVO_STOP_VALUE);
    Serial.printf("Servo: ARRET FINAL %d\n", SERVO_STOP_VALUE);
    
    Serial.println("Porte refermee.");
}