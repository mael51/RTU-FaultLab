# Changelog

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