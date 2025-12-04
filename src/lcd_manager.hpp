#ifndef LCD_MANAGER_HPP
#define LCD_MANAGER_HPP
#include <LiquidCrystal_I2C.h>
#include "config.hpp" 
#include "network_manager.hpp" // Ajouté pour accéder à UID_SERRURE dans l'implémentation

// Déclaration externe du pointeur LCD
extern LiquidCrystal_I2C *lcd;

void initialiser_lcd();

void lcd_message_attente(); // <-- Déclaration finale corrigée

#endif