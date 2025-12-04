#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>

// ==============================================================================
// CONFIGURATION DU MATÉRIEL ET DES BROCHES
// ==============================================================================

// --- RFID RC522 ---
#define RST_PIN 4
#define SS_PIN 5

// --- LCD I2C ---
const byte LCD_ADDR_DEFAULT = 0x27; 
// Note: Si 0x27 ne fonctionne pas, essayez 0x3F ou 0x20

// --- SERVOMOTEUR ---
const int BROCHE_SERVO = 26;
const int SERVO_STOP_VALUE = 1500; 
const int SERVO_OUVERTURE_VITESSE = 1700;
const int SERVO_FERMETURE_VITESSE = 1300;
const int DUREE_MOUVEMENT_MS = 1500; 

// ==============================================================================
// CONFIGURATION DU RÉSEAU (WiFi et MQTT)
// Les déclarations 'extern' indiquent que les définitions sont dans un fichier .cpp.
// ==============================================================================
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const char* MQTT_SERVER;
extern const int MQTT_PORT; // int peut aussi poser problème, mieux de le déclarer extern

// Topics et UID
extern const char* TOPIC_SUB_RESPONSE; 
extern const char* TOPIC_PUB_RFID;         
extern const char* TOPIC_PAIRING_REQ;
extern const char* TOPIC_PAIRING_CONFIRM;
extern const char* NOUVEL_UID_SERRURE;

#endif