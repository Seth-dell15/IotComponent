#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include "esp_system.h"

String genererCleAppareillage(int longueurOctets);
void ouvrirPorte();
void lcd_message_attente(); 
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);

const char* WIFI_SSID = "A16 de Hugo";
const char* WIFI_PASSWORD = "Jambon1234";

const char* MQTT_SERVER = "broker.emqx.io";
const int MQTT_PORT = 1883;

const char* TOPIC_SUB = "serrure/response";
const char* TOPIC_PUB = "serrure/rfid";

const char* TOPIC_PAIRING_REQ = "serrure/pairing/request";
const char* TOPIC_PAIRING_CONFIRM = "serrure/pairing/confirm";

String UID_SERRURE = "A_APPARIER"; 
String CLE_APAREILLAGE_TEMPORAIRE = ""; 
const char* NOUVEL_UID_SERRURE = "01"; 

WiFiClient espClient;
PubSubClient client(espClient);

#define RST_PIN 4
#define SS_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(0x27, 16, 2); // ADRESSE I2C: Tentez 0x3F si 0x27 ne fonctionne pas

Servo monServo; 
const int brocheServo = 26;

const int SERVO_FERME_ANGLE = 0;
const int SERVO_OUVERT_ANGLE = 90;
const int SERVO_VITESSE_DELAI = 15;

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

    Serial.print("Message recu sur ");
    Serial.print(topic);
    Serial.print(" : ");
    Serial.println(message);

    if (strcmp(topic, TOPIC_SUB) == 0) {
        lcd.clear();
        lcd.setCursor(0, 0); 
        
        if (cleanMessage == "OK") {
            lcd.print("ACCES AUTORISES");
            lcd.setCursor(0, 1); 
            lcd.print("Porte ouverte!");
            ouvrirPorte();
        } else if (cleanMessage == "REFUSE") {
            lcd.print("ACCES REFUSES");
            lcd.setCursor(0, 1); 
            lcd.print("Mauvaise carte.");
        } else {
            lcd.print(cleanMessage); 
            lcd.setCursor(0, 1);
            lcd.print("Erreur du serveur");
        }
    }
    
    else if (strcmp(topic, TOPIC_PAIRING_REQ) == 0) {
        if (CLE_APAREILLAGE_TEMPORAIRE.length() > 0 && cleanMessage == CLE_APAREILLAGE_TEMPORAIRE) {
            
            const size_t CAPACITY_PAIRING = JSON_OBJECT_SIZE(2);
            StaticJsonDocument<CAPACITY_PAIRING> doc;
            
            doc["uid_serrure"] = NOUVEL_UID_SERRURE;
            doc["status"] = "ok";
            
            String jsonConfirm;
            serializeJson(doc, jsonConfirm);

            client.publish(TOPIC_PAIRING_CONFIRM, jsonConfirm.c_str());
            Serial.print("Appareillage confirme envoye: ");
            Serial.println(jsonConfirm);

            UID_SERRURE = NOUVEL_UID_SERRURE;
            CLE_APAREILLAGE_TEMPORAIRE = "";

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("APPARIEMENT OK!");
            lcd.setCursor(0, 1);
            delay(5000);
        } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Echec appairage");
            delay(2000);
        }

        lcd_message_attente();
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Connexion au broker MQTT... ");
        if (client.connect("ESP32_Client_Serrure")) { 
            Serial.println("OK !");
            
            // Abonnement aux topics standard
            client.subscribe(TOPIC_SUB);
            Serial.print("Abonne a: "); Serial.println(TOPIC_SUB);
            
            // Abonnement au topic d'appareillage
            client.subscribe(TOPIC_PAIRING_REQ);
            Serial.print("Abonne a: "); Serial.println(TOPIC_PAIRING_REQ);
            
        } else {
            Serial.print("Echec rc=");
            Serial.print(client.state());
            Serial.println(" -> nouvel essai dans 2s");
            delay(2000);
        }
    }
}

// Reinitialise l'affichage LCD a l'etat d'attente normal
void lcd_message_attente() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Systeme pret (ID:");
    lcd.print(UID_SERRURE);
    lcd.print(")");
    lcd.setCursor(0, 1);
    lcd.print("Presentez carte");
}


// Génère une chaîne hexadécimale aléatoire
String genererCleAppareillage(int longueurOctets) {
    String cle = "";
    randomSeed(esp_random()); 

    for (int i = 0; i < longueurOctets; i++) {
        byte octetAleatoire = random(0, 256);
        String hex = String(octetAleatoire, HEX);

        if (hex.length() < 2) {
            hex = "0" + hex;
        }
        cle += hex;
    }
    cle.toUpperCase();
    return cle;
}

void ouvrirPorte(){
    Serial.println("Ouverture de la porte...");
    
    for (int angle = SERVO_FERME_ANGLE; angle <= SERVO_OUVERT_ANGLE; angle++) {
        monServo.write(angle);
        delay(SERVO_VITESSE_DELAI); 
    }

    delay(2500); 

    for (int angle = SERVO_OUVERT_ANGLE; angle >= SERVO_FERME_ANGLE; angle--) {
        monServo.write(angle);
        delay(SERVO_VITESSE_DELAI);
    }
    
    Serial.println("Porte refermee.");
}

void setup() {
    Serial.begin(115200);

    const int LONGUEUR_CLE = 8;
    CLE_APAREILLAGE_TEMPORAIRE = genererCleAppareillage(LONGUEUR_CLE);
    
    Serial.print("Cle d'appareillage a entrer sur le serveur pour le provisioning : ");
    Serial.println(CLE_APAREILLAGE_TEMPORAIRE);

    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Lecteur RFID pret.");

    Serial.print("Connexion au WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connecte !");

    Wire.begin(); 
    lcd.init();
    lcd.backlight();
    
    // Test d'initialisation du LCD (utile pour regler le contraste)
    lcd.setCursor(0, 0);
    lcd.print("LCD Test OK");
    lcd.setCursor(0, 1);
    lcd.print("Contraste / I2C?");
    delay(3000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cle Appareillage:");
    lcd.setCursor(0, 1);
    lcd.print(CLE_APAREILLAGE_TEMPORAIRE);

    delay(40000); // Laisse 20s pour noter la cle

    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
    
    lcd_message_attente();
}

void loop() {

    if (!client.connected()) reconnect();
    client.loop();

    if (UID_SERRURE == "A_APPARIER") {
        delay(100); 
        return;
    }

    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    String uidText = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) uidText += "0";
        uidText += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidText.toUpperCase();

    DynamicJsonDocument doc(256); 

    doc["uid_carte"] = uidText;
    doc["uid_serrure"] = UID_SERRURE;

    String jsonString;
    serializeJson(doc, jsonString);

    if (client.publish(TOPIC_PUB, jsonString.c_str())) {
        Serial.print("Message JSON envoye: ");
        Serial.println(jsonString);
    } else {
        Serial.println("ECHEC publication MQTT.");
    }
    
    // Arrêt de la carte
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    // Delai pour eviter les lectures multiples du rfid
    delay(500); 
}