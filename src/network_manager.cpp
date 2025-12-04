#include "network_manager.hpp" 
#include "config.hpp" 
#include "esp_system.h" 
#include <ArduinoJson.h> 
#include "lcd_manager.hpp" 
#include "servo_manager.hpp" // Rétabli pour l'appel à ouvrirPorte

// Définition des variables globales
WiFiClient espClient;
PubSubClient client(espClient);
String UID_SERRURE = "A_APPARIER"; 
String CLE_APAREILLAGE_TEMPORAIRE = ""; 

/**
 * Fonction de callback exécutée lors de la réception d'un message MQTT.
 */
void callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    message.trim(); 

    String cleanMessage = message;
    if (cleanMessage.startsWith("\"") && cleanMessage.endsWith("\"") && cleanMessage.length() >= 2) {
        cleanMessage = cleanMessage.substring(1, cleanMessage.length() - 1);
        cleanMessage.trim();
    }

    Serial.printf("Message recu sur %s : %s\n", topic, cleanMessage.c_str());

    // ----------------------------------------------------------------------
    // 1. GESTION DE LA RÉPONSE D'ACCÈS
    // ----------------------------------------------------------------------
    if (strcmp(topic, TOPIC_SUB_RESPONSE) == 0) {
        lcd->clear();
        lcd->setCursor(0, 0); 
        
        if (cleanMessage == "OK") {
            lcd->print("ACCES AUTORISES");
            lcd->setCursor(0, 1); 
            lcd->print("Ouverture loggee."); 
            ouvrirPorte(); // Appel à la fonction de simulation (ou future réactivation)
        } else if (cleanMessage == "REFUSE") {
            lcd->print("ACCES REFUSES");
            lcd->setCursor(0, 1); 
            lcd->print("Mauvaise carte.");
        } else {
            lcd->print(cleanMessage); 
            lcd->setCursor(0, 1);
            lcd->print("Erreur du serveur");
        }
        delay(2000);
        lcd_message_attente(); // Correction: Pas d'argument ici
    }
    
    // ----------------------------------------------------------------------
    // 2. GESTION DE L'APPAREILLAGE (PAIRING)
    // ----------------------------------------------------------------------
    else if (strcmp(topic, TOPIC_PAIRING_REQ) == 0) {
        if (CLE_APAREILLAGE_TEMPORAIRE.length() > 0 && cleanMessage == CLE_APAREILLAGE_TEMPORAIRE) {
            
            StaticJsonDocument<JSON_OBJECT_SIZE(2)> doc;
            
            doc["uid_serrure"] = NOUVEL_UID_SERRURE;
            doc["status"] = "ok";
            
            String jsonConfirm;
            serializeJson(doc, jsonConfirm);

            client.publish(TOPIC_PAIRING_CONFIRM, jsonConfirm.c_str());
            Serial.print("Appareillage confirme envoye: ");
            Serial.println(jsonConfirm);

            UID_SERRURE = NOUVEL_UID_SERRURE;
            CLE_APAREILLAGE_TEMPORAIRE = "";

            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("APPARIEMENT OK!");
            lcd->setCursor(0, 1);
            lcd->print("ID: 01");
            delay(5000);
            lcd_message_attente(); // Correction: Pas d'argument ici
        } else {
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Echec appairage");
            delay(2000);
            lcd_message_attente(); // Correction: Pas d'argument ici
        }
    }
}


/**
 * Génère une chaîne hexadécimale aléatoire pour l'appareillage.
 */
String genererCleAppareillage(int longueurOctets) {
    String cle = "";
    randomSeed(esp_random()); 

    for (int i = 0; i < longueurOctets; i++) {
        byte octetAleatoire = random(0, 256);
        char buffer[3];
        sprintf(buffer, "%02X", octetAleatoire); 
        cle += buffer;
    }
    return cle;
}

/**
 * Initialise le WiFi et le client MQTT.
 */
void initialiser_reseau() {
    Serial.print("Connexion au WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connecte !");
    
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback); 
}

/**
 * Reconnexion au broker MQTT si la connexion est perdue.
 */
void reconnect() {
    while (!client.connected()) {
        Serial.print("Connexion au broker MQTT... ");
        if (client.connect("ESP32_Client_Serrure")) { 
            Serial.println("OK !");
            
            // Abonnement aux topics
            client.subscribe(TOPIC_SUB_RESPONSE);
            client.subscribe(TOPIC_PAIRING_REQ);
            
        } else {
            Serial.printf("Echec rc=%d -> nouvel essai dans 2s\n", client.state());
            delay(2000);
        }
    }
}