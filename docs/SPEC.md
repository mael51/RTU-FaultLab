# SPEC — RTU-FaultLab

Spécification fonctionnelle et technique (synthèse). Source détaillée : `Cahier_des_charges_RTU_FaultLab.md`.

## 1. Périmètre MVP

- Esclave Modbus RTU : FC03 (Read Holding Registers), FC06 (Write Single Register).
- Contrôle de deux sorties logiques, état sûr, timeout heartbeat.
- Transport virtuel PC + injecteur de défauts déterministe.
- Firmware C++17 portable + adapter hôte ; architecture d'adapter STM32.
- Pytest, CLI, scénarios YAML, rapports JSON/JUnit/HTML, CI.

Hors périmètre initial : Modbus complet, relais secteur, réseau multi-maîtres, GUI/cloud, certification.

## 2. Comportement général

- Démarrage en état SAFE : sorties OFF.
- Écoute des trames destinées à l'adresse esclave (défaut 0x01).
- Trame traitée uniquement si longueur, adresse, fonction et CRC valides.
- Toute requête valide met à jour l'horodatage de dernière activité ; heartbeat explicite (FC06 sur 0x0005) rafraîchit la fenêtre.
- Expiration du heartbeat : sorties OFF, status expose le motif.
- Erreur de protocole : jamais d'écriture partielle ni de commande non autorisée.

## 3. Carte des registres

Voir `firmware/core/registers.hpp` (source unique).

| Adresse | Nom | Accès | Sémantique |
|---|---|---|---|
| 0x0000 | CONTROL_WORD | R/W (FC06) | Bit0 sortie1, bit1 sortie2 ; autres bits réservés et rejetés |
| 0x0001 | OUTPUT_STATE | RO (FC03) | Image des sorties réellement appliquées (bits 0/1) |
| 0x0002 | DEVICE_STATUS | RO (FC03) | Bit0 READY, bit1 SAFE_TIMEOUT, bit2 PROTOCOL_ERROR, bit3 RESET_CAUSE |
| 0x0003 | UPTIME_SECONDS | RO (FC03) | Compteur monotone (modulo/saturation documenté) |
| 0x0004 | LAST_ERROR | RO (FC03) | Code de la dernière erreur diagnostique |
| 0x0005 | HEARTBEAT | WO (FC06) | Écriture séquencée, rafraîchit le délai de sécurité |

## 4. Paramètres série par défaut

19200 bit/s, 8 bits, parité paire, 1 stop (8E1), configurables.

## 5. Timeout heartbeat

1000 ms, configurable, testé avec horloge injectée.

## 6. Modbus RTU (sous-ensemble)

Esclave RTU, FC03, FC06, exceptions nécessaires, CRC-16/Modbus, gestion des limites de trame (buffer 256 octets). Silences inter-trame/inter-caractère gérés par la couche transport avec horloge simulée.

## 7. Codes d'exception Modbus

0x01 fonction illégale, 0x02 adresse de données illégale, 0x03 valeur de données illégale.

## 8. Sécurité fonctionnelle (pédagogique)

Initialisation sorties OFF, aucune sortie activée par trame invalide, retour OFF après expiration heartbeat, récupération selon règle explicitement spécifiée. Non certifié.

## 9. Transport

Interface commune `IByteTransport` : loopback, série (PySerial), RS-485. Mode Host : enveloppes hex `REQ/RESP` sur stdin/stdout, octets bruts réinjectés dans le parser, logs sur stderr.