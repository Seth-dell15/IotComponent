#include "config.hpp"

// ==============================================================================
// Définition UNIQUE des constantes réseau et MQTT
// Ce fichier est inclus dans la compilation une seule fois.
// ==============================================================================

// WiFi
const char* WIFI_SSID = "A16 de Hugo";
const char* WIFI_PASSWORD = "Jambon1234";

// MQTT
const char* MQTT_SERVER = "broker.emqx.io";
const int MQTT_PORT = 1883;

// Topics et UID
const char* TOPIC_SUB_RESPONSE = "serrure/response"; 
const char* TOPIC_PUB_RFID = "serrure/rfid";         
const char* TOPIC_PAIRING_REQ = "serrure/pairing/request";
const char* TOPIC_PAIRING_CONFIRM = "serrure/pairing/confirm";
const char* NOUVEL_UID_SERRURE = "01";