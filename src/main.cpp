#include <Arduino.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h> // Librairie LCD I2C
#include <Servo.h>

#define RST_PIN 4
#define SS_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(0x27, 16, 2); 

Servo monServo; 
const int brocheServo = 26; 
const int angle_ouvert = 90;
const int angle_ferme = 0;

void lcd_message_attente() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Systeme pret (ID:");
    lcd.print(")");
    lcd.setCursor(0, 1);
    lcd.print("Presentez carte");
}

void ouvrirPorte(){
    Serial.println("Ouverture de la porte...");
    monServo.write(angle_ouvert);
    delay(3000); 
    monServo.write(angle_ferme);
    delay(3000);  
}

void setup() {
  Serial.begin(115200); 

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Lecteur RFID pret.");
    
  Wire.begin(); 
  lcd.init();
  lcd.backlight();
    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cle Appareillage:");
  lcd.setCursor(0, 1);
  lcd.print("Attente...");

  delay(40000);
}

void loop() {
  ouvrirPorte();
  lcd_message_attente();
  delay(40000);
}

