# GPS-RTRIP-V2

## Description

**GPS-RTRIP-V2** est un dispositif autonome de suivi de randonnées (2ᵉ version) permettant d’enregistrer les déplacements grâce à un système GPS et différents capteurs embarqués.

Cette version repose sur un **ESP32**, offrant de meilleures performances ainsi que des capacités de communication sans fil.  
Les données de parcours sont enregistrées sur une **carte mémoire** et peuvent désormais être **transmises automatiquement vers une API REST personnelle**, afin de permettre la consultation des randonnées via un autre projet complémentaire : une application web nommée **HikingBook** (en cours de finalisation).

Le projet s’inscrit dans une démarche d’apprentissage et de mise en pratique de l’électronique embarquée, du développement logiciel et des architectures client–serveur.

---

## Objectifs

- Enregistrer les parcours de randonnée (positions GPS, durée, etc.)
- Stocker localement les données sur carte microSD
- Envoyer les données vers une API REST via Wi-Fi
- Permettre la consultation des randonnées via une application web dédiée

---

## Outils utilisés

- Visual Studio Code  
- Arduino IDE  
- Git  
- Tinkercad  
- Ultimaker Cura  

---

## Technologies

- **C++**
- **NMEA (GPS)**
- **Wi-Fi / API REST**

---

## Équipements

- ESP32 Devkit C  
- Module Micro-SDHC (SPI)  
- Module GPS NEO-M8L  
- Boutons poussoirs  
- Connecteur USB-C  
- DHT11 (capteur de température et d’humidité)

---

