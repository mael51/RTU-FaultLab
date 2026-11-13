# Documentation RTU-FaultLab

Plan en phases (comme EmbeddedSense). Chaque phase a un fichier synthèse (`phase_X_*.md`) et une documentation détaillée (`phase_X_detaillee.md`).

| Phase | Thème | Jalon | Statut |
|---|---|---|---|
| 0 | Mise en place de l'environnement | M0 | ✅ |
| 1 | Spécification et carte des registres | M0 | ✅ |
| 2 | Cœur Modbus RTU (CRC, parser, FC03/FC06) | M1 | ✅ |
| 3 | Carte des registres et handlers | M1 | ✅ |
| 4 | Machine à états, sorties, sécurité | M2 | ✅ |
| 5 | Transport et DUT hôte (SIL) | M2 | ✅ |
| 6 | Injection de défauts déterministe | M3 | ✅ |
| 7 | Runner Python et campagnes YAML | M3 | ✅ |
| 8 | Rapports et traçabilité | M4 | ✅ |
| 9 | Qualité, sanitizers, CI | M4 | ✅ |
| 10 | Portage STM32 / FreeRTOS | M5 | ✅ |
| 11 | Finalisation et démonstration | — | ✅ |

Règle d'or : ne passer à la phase suivante sans valider la checklist de la phase en cours.