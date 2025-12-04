#include "lcd_manager.hpp" 
#include <Wire.h>
#include "network_manager.hpp" // Inclusion pour accéder à UID_SERRURE

// Définition de l'objet LCD
LiquidCrystal_I2C *lcd = nullptr; 

/**
 * Initialise le bus I2C et l'objet LCD.
 */
void initialiser_lcd() {
    Wire.begin(); 
    lcd = new LiquidCrystal_I2C(LCD_ADDR_DEFAULT, 16, 2); 
    lcd->init(); 
    lcd->backlight();
    
    // Test d'initialisation
    lcd->setCursor(0, 0);
    lcd->print("Systeme de Serrure");
    lcd->setCursor(0, 1);
    lcd->print("LCD Init OK");
    delay(1000);
}

/**
 * Affiche le message par défaut en attente de carte RFID.
 */
void lcd_message_attente() {
    if (!lcd) return; 
    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print("Systeme pret (ID:");
    // Utilisation de la variable globale UID_SERRURE
    lcd->print(UID_SERRURE); 
    lcd->print(")");
    lcd->setCursor(0, 1);
    lcd->print("Presentez carte");
}