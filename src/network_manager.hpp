#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.hpp" 

// Déclaration externe des variables de connexion et d'état
extern WiFiClient espClient;
extern PubSubClient client;
extern String UID_SERRURE; 
extern String CLE_APAREILLAGE_TEMPORAIRE; 

void initialiser_reseau();
void reconnect();
String genererCleAppareillage(int longueurOctets);

#endif