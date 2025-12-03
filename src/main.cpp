#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> // Librairie LCD I2C
#include <Servo.h>
#include <ArduinoJson.h>
#include "esp_system.h" // Pour l'aleatoire robuste de l'ESP32


// --- PROTOTYPES DES FONCTIONS ---
String genererCleAppareillage(int longueurOctets);
void ouvrirPorte();
void lcd_message_attente(); 
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);


// ------------ CONFIG WIFI ------------
const char* WIFI_SSID = "A16 de Hugo";
const char* WIFI_PASSWORD = "Jambon1234";

// ------------ CONFIG MQTT ------------
const char* MQTT_SERVER = "broker.emqx.io";
const int MQTT_PORT = 1883;

// Topics Standards
const char* TOPIC_SUB = "serrure/response";      // Topic pour recevoir OK/REFUSE
const char* TOPIC_PUB = "serrure/rfid";          // Topic pour envoyer UID

// Topics d'Appareillage (Provisioning)
const char* TOPIC_PAIRING_REQ = "serrure/pairing/request"; // Topic pour ecouter la cle du serveur
const char* TOPIC_PAIRING_CONFIRM = "serrure/pairing/confirm"; // Topic pour confirmer l'appairage

// ------------ VARIABLES GLOBALES D'ETAT ------------
// ID permanent de la serrure. Initialise a une valeur temporaire.
String UID_SERRURE = "A_APPARIER"; 
// Cle temporaire generee au demarrage pour l'appairage
String CLE_APAREILLAGE_TEMPORAIRE = ""; 
// L'ID cible a attribuer apres appareillage reussi (selon votre requete)
const char* NOUVEL_UID_SERRURE = "01"; 

WiFiClient espClient;
PubSubClient client(espClient);

// ------------ BROCHES RFID ------------
#define RST_PIN 4
#define SS_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ------------ LCD I2C ------------
// ADRESSE I2C: Tentez 0x3F si 0x27 ne fonctionne pas.
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- Définitions du Servomoteur Hitec HS-311 (ANGULAIRE) ---
Servo monServo; 
const int brocheServo = 26; // GPIO 26 sur ESP32

// Constantes pour un servomoteur ANGULAIRE (0-180 deg)
const int SERVO_FERME_ANGLE = 0;   // Position ou le verrou est engage
const int SERVO_OUVERT_ANGLE = 90; // Position ou le verrou est retracte
const int SERVO_VITESSE_DELAI = 15; // Delai en ms pour chaque pas (plus c'est grand, plus c'est lent)


// ------------ CALLBACK MQTT ------------
void callback(char* topic, byte* payload, unsigned int length) {
    // Lecture du message recu
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    message.trim(); 

    // Nettoyage: retirer guillemets entourantes si present ("...") car certains clients envoient la chaîne entre guillemets
    String cleanMessage = message;
    if (cleanMessage.startsWith("\"") && cleanMessage.endsWith("\"") && cleanMessage.length() >= 2) {
        cleanMessage = cleanMessage.substring(1, cleanMessage.length() - 1);
        cleanMessage.trim();
    }

    Serial.print("Message recu sur ");
    Serial.print(topic);
    Serial.print(" : ");
    Serial.println(message);

    // --- CAS 1: REPONSE D'ACCES (OK/REFUSE) ---
    if (strcmp(topic, TOPIC_SUB) == 0) {
        lcd.clear();
        lcd.setCursor(0, 0); 
        
        if (cleanMessage == "OK") {
            lcd.print("ACCES AUTORISES");
            lcd.setCursor(0, 1); 
            lcd.print("Porte ouverte!");
            ouvrirPorte(); // Action du servo
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
    
    // --- CAS 2: REQUETE D'APPAREILLAGE ---
    else if (strcmp(topic, TOPIC_PAIRING_REQ) == 0) {
        // Verification si la cle temporaire est valide et correspond au payload nettoye
        if (CLE_APAREILLAGE_TEMPORAIRE.length() > 0 && cleanMessage == CLE_APAREILLAGE_TEMPORAIRE) {
            
            // 1. Preparation du message JSON de confirmation (Taille fixe, on peut utiliser Static)
            const size_t CAPACITY_PAIRING = JSON_OBJECT_SIZE(2);
            StaticJsonDocument<CAPACITY_PAIRING> doc;
            
            // Construction du JSON: {"uid_serrure": "01", "status": "ok"}
            doc["uid_serrure"] = NOUVEL_UID_SERRURE;
            doc["status"] = "ok";
            
            String jsonConfirm;
            serializeJson(doc, jsonConfirm);

            // 2. Publication de la confirmation
            client.publish(TOPIC_PAIRING_CONFIRM, jsonConfirm.c_str());
            Serial.print("Appareillage confirme envoye: ");
            Serial.println(jsonConfirm);

            // 3. Mise a jour de l'ID permanent de la serrure (DEBLOQUE LE SCAN RFID)
            UID_SERRURE = NOUVEL_UID_SERRURE;
            CLE_APAREILLAGE_TEMPORAIRE = ""; // Invalider la cle temporaire

            // 4. Affichage LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("APPARIEMENT OK!");
            lcd.setCursor(0, 1);
            lcd.print("ID: ");
            lcd.print(NOUVEL_UID_SERRURE);
            delay(5000); // Afficher la confirmation 5s
        } else {
            // Echec de l'appareillage (mauvaise cle)
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Echec appairage");
            delay(2000);
        }

        // Remettre LCD en mode de fonctionnement normal
        lcd_message_attente();
    }
}

// ------------ RECONNEXION MQTT ------------
void reconnect() {
    while (!client.connected()) {
        Serial.print("Connexion au broker MQTT... ");
        // Le nom du client doit etre unique pour le broker
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

// --- FONCTIONS SECONDAIRES ---

/**
 * Reinitialise l'affichage LCD a l'etat d'attente normal.
 */
void lcd_message_attente() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Systeme pret (ID:");
    lcd.print(UID_SERRURE);
    lcd.print(")");
    lcd.setCursor(0, 1);
    lcd.print("Presentez carte");
}


/**
 * Génère une chaîne hexadécimale aléatoire.
 */
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

/**
 * Deplace le servomoteur de la position FERMEE a OUVERTE, puis le ramene a FERMEE
 * en gerant la vitesse par pas.
 */
void ouvrirPorte(){
    Serial.println("Ouverture de la porte...");
    
    // 1. OUVERTURE LENTE
    for (int angle = SERVO_FERME_ANGLE; angle <= SERVO_OUVERT_ANGLE; angle++) {
        monServo.write(angle);
        delay(SERVO_VITESSE_DELAI); 
    }

    // Maintient la position ouverte
    delay(2500); 

    // 2. FERMETURE LENTE
    for (int angle = SERVO_OUVERT_ANGLE; angle >= SERVO_FERME_ANGLE; angle--) {
        monServo.write(angle);
        delay(SERVO_VITESSE_DELAI);
    }
    
    Serial.println("Porte refermee.");
}

void setup() {
    Serial.begin(115200);

    // Generation et stockage de la cle d'appareillage temporaire
    const int LONGUEUR_CLE = 8;
    CLE_APAREILLAGE_TEMPORAIRE = genererCleAppareillage(LONGUEUR_CLE);
    
    Serial.print("Cle d'appareillage a entrer sur le serveur pour le provisioning : ");
    Serial.println(CLE_APAREILLAGE_TEMPORAIRE);

    // Servo
    //monServo.attach(brocheServo);
    //monServo.write(SERVO_FERME_ANGLE); 

    // RFID
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("Lecteur RFID pret.");

    // WiFi (Connexion)
    Serial.print("Connexion au WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connecte !");
    
    // LCD (Initialisation et debogage)
    Wire.begin(); 
    lcd.init();
    lcd.backlight();
    
    // Test d'initialisation du LCD (utile pour regler le contraste)
    lcd.setCursor(0, 0);
    lcd.print("LCD Test OK");
    lcd.setCursor(0, 1);
    lcd.print("Contraste / I2C?");
    delay(3000); 

    // Affichage initial de la cle d'appareillage
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cle Appareillage:");
    lcd.setCursor(0, 1);
    lcd.print(CLE_APAREILLAGE_TEMPORAIRE);

    // Laisse 20s pour noter la cle
    delay(40000);

    // Configuration et reconnexion MQTT
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
    
    // Affichage de l'attente apres le delay
    lcd_message_attente();
}

void loop() {

    if (!client.connected()) reconnect();
    client.loop();

    // BLOQUE LA LECTURE RFID TANT QUE L'APPAREILLAGE N'EST PAS TERMINE (UID_SERRURE est "A_APPARIER").
    if (UID_SERRURE == "A_APPARIER") {
        delay(100); 
        return;
    }

    // Attendre nouvelle carte
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    // --- 1. CONSTRUIRE L'UID ---
    String uidText = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) uidText += "0";
        uidText += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidText.toUpperCase();

    // --- 2. CREATION DU MESSAGE JSON ---
    // Utilise DynamicJsonDocument (256 octets) pour que la taille soit calculée à l'exécution.
    DynamicJsonDocument doc(256); 

    doc["uid_carte"] = uidText;
    doc["uid_serrure"] = UID_SERRURE; // Utilisation de l'UID permanent ("01")

    String jsonString;
    serializeJson(doc, jsonString);

    // --- 3. PUBLICATION VIA MQTT ---
    if (client.publish(TOPIC_PUB, jsonString.c_str())) {
        Serial.print("Message JSON envoye: ");
        Serial.println(jsonString);
    } else {
        Serial.println("ECHEC publication MQTT.");
    }
    
    // Stopper la carte et eviter spam
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    // Delai de debounce
    delay(500); 
}