# RTU-FaultLab

> **Banc de validation SIL pour firmware embarqué Modbus RTU — déterministe, traçable, sans carte.**

[![CI](https://github.com/mael51/RTU-FaultLab/actions/workflows/ci.yml/badge.svg)](https://github.com/mael51/RTU-FaultLab/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/mael51/RTU-FaultLab?label=release)](https://github.com/mael51/RTU-FaultLab/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C)
![Python](https://img.shields.io/badge/Python-3.9%2B-3776AB)

**RTU-FaultLab** est un banc automatisé de validation firmware pour un contrôleur embarqué
communiquant en **Modbus RTU**. Le firmware tourne sur un hôte (SIL) piloté par un banc
Python/Pytest qui injecte des défauts déterministes et vérifie le comportement attendu
avec traçabilité exigence ↔ test.

**Pourquoi ce projet ?** Valider un firmware embarqué demande habituellement une carte
physique, un câblage RS-485 et des essais manuels. RTU-FaultLab déplace cette validation
en simulation : même code métier, mêmes scénarios, résultats reproductibles en CI, sans
matériel. Le portage STM32 est préparé (stubs + guide) mais volontairement non annoncé
comme validé.

## Chiffres clés

| | |
|---|---|
| Tests C++ | **57** (GoogleTest, ASan/UBSan/TSan) |
| Tests Python | **33** (Pytest) |
| Scénarios de campagne | **7 smoke + 12 robustness** |
| Injections de défauts | **7** (5 TX + 2 RX), déterministes par seed |
| Couverture | capture lcov 2.x, filtrée, vérifiée non vide |
| CI | 4 jobs verts sur le SHA tagué (`v1.0.0`) |

## Architecture (SIL)

```text
┌──────────────┐   REQ <hex>    ┌──────────────────┐
│  Bench Python│ ─────────────► │  DUT host (C++17)│
│  (runner,    │                │  Modbus RTU,     │
│   oracle,    │ ◄───────────── │  Safety Ctrl,    │
│   campaigns) │   RESP <hex>   │  RegisterMap     │
└──────────────┘                └──────────────────┘
       │ injection TX/RX (drop, delay, corrupt, …)
       │ traçabilité exigence ↔ test (YAML)
       ▼
   rapports JUnit / JSON / HTML
```

- **Firmware** : C++17 portable, Modbus RTU (FC03 lecture, FC06 écriture), CRC16,
  machine de sûreté (BOOT_SAFE → READY → RUNNING → SAFE_TIMEOUT → RECOVERY_PENDING),
  application centralisée des sorties.
- **Banc** : Python/Pytest, runner CLI, campagnes YAML, injection déterministe par seed,
  oracle strict (CRC y compris exceptions, état inconnu ≠ PASS, réponses tardives purgées).
- **DUT hôte** : processus stdin/stdout, file bornée, décodage hex strict, arrêt propre.

## Statut

- **Validé (SIL)** : cœur Modbus RTU, machine de sûreté, DUT hôte, transport virtuel,
  injections TX/RX, oracle, traçabilité, campagnes, CI complète.
- **Non validé** : portage STM32 / RS-485 physique (architecture préparée, stubs + guide).
  Aucune validation matérielle n'est annoncée tant qu'aucun essai sur carte n'a été réalisé.

## Démarrage rapide

```bash
# 1. Build
cmake -G Ninja -S . -B build
cmake --build build --parallel

# 2. Tests C++
cd build && ./rtufaultlab_tests   # .exe sous Windows

# 3. Tests Python
pip install pytest pyyaml
python -m pytest tests/integration tests/unit/test_bench_runner.py -q

# 4. Campagnes
python bench/runner/cli.py bench/campaigns/smoke.yaml \
  --dut build/rtufaultlab_dut_host.exe --reports bench/reports
python bench/runner/cli.py bench/campaigns/robustness.yaml \
  --dut build/rtufaultlab_dut_host.exe --reports bench/reports

# 5. Démo
python bench/demo.py
```

> Sous Windows (MSYS2/UCRT64), préfixer le `PATH` avec `C:\msys64\ucrt64\bin` pour les
> DLL du compilateur. En cas d'échec de démarrage du DUT, le runner affiche un diagnostic.

## Injection de défauts

Chaque défaut déclare son **point d'application** : TX (requête PC → DUT) ou RX
(réponse DUT → PC). Toutes les injections sont déterministes (seed) et vérifiées :
le runner refuse un scénario dont le défaut configuré n'a pas été réellement appliqué.

| Défaut | Point | Effet observable |
|---|---|---|
| `drop` | TX | la requête n'atteint jamais le DUT → timeout |
| `delay_ms` | TX | la requête est retardée ; au-delà du timeout → timeout |
| `duplicate` | TX | la requête est envoyée deux fois |
| `corrupt_crc` | TX | CRC invalide → trame rejetée par le DUT |
| `truncate` | TX | trame incomplète → rejetée sans crash |
| `drop_response` | RX | la requête atteint le DUT, la réponse est perdue ; l'effet DUT reste vérifiable |
| `delay_response_ms` | RX | la réponse arrive en retard ; au-delà du timeout → timeout |

Limites : injection logique au niveau IPC (stdin/stdout du DUT hôte), pas d'injection
électrique RS-485 ; un seul défaut par scénario ; après un timeout, la réponse tardive
éventuelle est purgée avant la requête suivante (resync).

## Documentation

- `docs/README.md` — plan en phases
- `docs/SPEC.md` — spécification
- `docs/hardware_guide.md` — portage STM32
- `CHANGELOG.md` — historique des releases

## Périmètre V1.0

- **Simulé (SIL)** : firmware hôte, transport virtuel, injection de défauts,
  machine de sûreté, traçabilité, CI.
- **Matériel** : portage STM32 optionnel, non validé sans carte.
- **Hors périmètre** : CAN/CAN-FD, interface graphique, injection électrique,
  certification (DO-178C, MISRA, IEC 61508).

## Licence

MIT — voir [LICENSE](LICENSE).