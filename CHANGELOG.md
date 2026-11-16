# Changelog

## [1.0.0-rc1] - 2026-11-16

Release candidate **Host/SIL validated** — CI 100 % verte sur le commit taggé.

### Fixed (V1-B1 à V1-B5)
- **V1-B1** Handler Modbus : vérification de la capacité de réponse sur **toutes**
  les branches (exceptions et FC06 incluses) ; aucune mutation de registre si la
  réponse ne peut pas être construite ; 8 tests de capacité limites ajoutés.
- **V1-B2** Machine de sûreté : tests aux frontières du watchdog (999/1000/1001 ms,
  convention « expiré » si `elapsed > timeout_ms`), rafraîchissement juste avant
  expiration, invariant OUTPUT_STATE ↔ GPIO après commande valide et timeout.
- **V1-B3** Oracle/transport : CRC vérifié aussi sur les réponses d'exception ;
  assertion sur état inconnu (`output_state: None`) ne peut jamais PASS ;
  resync après timeout (purge des réponses tardives avant la requête suivante).
- **V1-B4** Entrées bornées et injection vérifiable : décodage hexadécimal strict
  du DUT (rejet non-hex/impair/surdimensionné), lignes IPC bornées
  (`MAX_IPC_LINE_CHARS`) ; défauts RX `drop_response` / `delay_response_ms`
  déclarés puis appliqués par le runner ; scénarios TC-009/010/011 avec
  assertion DUT observable ; chaque défaut configuré est vérifié appliqué.
- **V1-B5** CI/rapports : clang-format épinglé 20.1.0, PATH UCRT64 exporté pour
  les étapes Python/DUT, capture lcov sans `--no-external` (cause racine de la
  capture vide) + vérification « coverage non vide », `--check-level=exhaustive`
  pour cppcheck, step Robustness auto-suffisant en dépendances.

### Known limitations
- Validation STM32/RS-485 physique non réalisée (architecture préparée uniquement).
- Portée Modbus limitée à FC03/FC06 (MVP assumé).

## [Unreleased] — stabilisation vers V1.0

### Fixed
- Machine de sûreté formalisée (BOOT_SAFE → READY → RUNNING → SAFE_TIMEOUT →
  RECOVERY_PENDING → RUNNING) ; le heartbeat seul ne réarme plus les sorties.
- Application des sorties centralisée dans `SafetyController` (cohérence
  CONTROL_WORD / OUTPUT_STATE / GPIO).
- DUT hôte : boucle principale sans condition toujours vraie, queue bornée avec
  diagnostic de débordement, arrêt propre et code de retour fiable.
- Transport virtuel : lecteur stdout permanent unique, file thread-safe, arrêt
  propre (terminate → kill → join), suppression du chemin MinGW codé en dur.
- Handler Modbus : accès uniquement via `RegisterMap` (suppression de l'accès
  raw), cas limites FC03/FC06 testés (quantité 0, hors plage, bits réservés).
- Oracle : vérifie CRC, absence d'exception sur succès, code d'exception, adresse
  esclave, code fonction, valeur d'écho FC06 et valeurs FC03.
- Traçabilité : distinction défini/exécuté, `INVALID_REFERENCE` réellement
  détecté, violations MUST bloquantes (`COVERED_FAIL`, `NOT_COVERED`).
- CI : quoting `find` corrigé (job Format), capture lcov robuste pour lcov 2.x,
  job ThreadSanitizer ajouté, artefacts de rapports publiés en cas d'échec.
- CMake : placeholders supprimés, GoogleTest verrouillé par `URL_HASH`, option
  TSan, timeouts CTest.
- README : statut honnête (SIL vs matériel), commandes exactes, prérequis
  explicites, licence référencée.

### Known limitations
- Validation STM32/RS-485 physique non réalisée (architecture préparée uniquement).
- Portée Modbus limitée à FC03/FC06 (MVP assumé).

## [0.1.0] - 2026-11-12

### Added
- Firmware C++17 portable : Modbus RTU (FC03, FC06), CRC, parser, registres, sécurité, heartbeat.
- Adapter hôte : loopback, GPIO mock, horloge injectable, DUT host (stdin/stdout).
- Banc Python : runner CLI, campagnes YAML, injection de défauts déterministe.
- Rapports JUnit XML, JSON, HTML, traçabilité.
- CI GitHub Actions (build, tests, format, sanitizers, smoke).
- Architecture adapter STM32 (stubs) et guide matériel.
- Documentation par phase (0–11) avec diagrammes Mermaid.