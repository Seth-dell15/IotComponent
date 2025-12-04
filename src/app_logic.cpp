#include "app_logic.hpp"
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

// Inclusion des modules de gestion
#include "config.hpp"
#include "lcd_manager.hpp"
#include "servo_manager.hpp" // Rétabli
#include "network_manager.hpp"

// --- RFID RC522 (Variable globale du module AppLogic) ---
MFRC522 mfrc522(SS_PIN, RST_PIN);


// =============================================================
// Implémentation des fonctions de l'application
// =============================================================

/**
 * Contient toute la logique d'initialisation de l'application (ancien setup()).
 */
void AppLogic::initialize() {
    Serial.begin(115200);
    
    // 1. Initialisation des composants matériels
    initialiser_servo(); // Rétabli pour l'initialisation des broches
    initialiser_lcd();

    // 2. Initialisation du Réseau et des variables globales
    const int LONGUEUR_CLE = 8;
    CLE_APAREILLAGE_TEMPORAIRE = genererCleAppareillage(LONGUEUR_CLE);
    
    Serial.print("Cle d'appareillage a entrer sur le serveur pour le provisioning : ");
    Serial.println(CLE_APAREILLAGE_TEMPORAIRE);

    initialiser_reseau(); // Connecte le WiFi, configure MQTT et définit le callback
    
    // 3. Initialisation du RFID
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Lecteur RFID pret.");
    
    // 4. Affichage de la clé d'appareillage initiale
    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print("Cle Appareillage:");
    lcd->setCursor(0, 1);
    lcd->print(CLE_APAREILLAGE_TEMPORAIRE);

    delay(40000); // Laisse 40s pour noter la cle

    lcd_message_attente(); // Correction: Pas d'argument ici
}


/**
 * Contient toute la logique d'exécution cyclique (ancien loop()).
 */
void AppLogic::execute() {
    // 1. Maintien de la connexion MQTT
    if (!client.connected()) reconnect();
    client.loop(); // Gère les messages entrants et appelle le callback

    // 2. Mode Appareillage (empêche la lecture RFID)
    if (UID_SERRURE == "A_APPARIER") {
        // Rafraîchit l'affichage de la clé d'appairage pendant l'attente
        lcd->setCursor(0, 0);
        lcd->print("Cle Appareillage:");
        lcd->setCursor(0, 1);
        lcd->print(CLE_APAREILLAGE_TEMPORAIRE);
        delay(100); 
        return;
    }

    // 3. Lecture RFID
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    // Récupération de l'UID et formatage
    String uidText = "";
    char buffer[3];
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        sprintf(buffer, "%02X", mfrc522.uid.uidByte[i]);
        uidText += buffer;
    }
    uidText.toUpperCase();

    // 4. Création et publication du message JSON
    DynamicJsonDocument doc(256); 
    doc["uid_carte"] = uidText;
    doc["uid_serrure"] = UID_SERRURE;

    String jsonString;
    serializeJson(doc, jsonString);

    if (client.publish(TOPIC_PUB_RFID, jsonString.c_str())) {
        Serial.print("Message JSON envoye: ");
        Serial.println(jsonString);
    } else {
        Serial.println("ECHEC publication MQTT.");
    }
    
    // 5. Arrêt de la carte et délai anti-doublon
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(500); 
}