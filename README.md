# RTU-FaultLab

Banc automatisé de validation firmware avec injection de défauts Modbus RTU / RS-485.

> Firmware C++17 portable • banc de test Python/Pytest • SIL sans matériel

## Statut

Version cible **V1.0 — Host/SIL validated** : le firmware hôte, le banc Python, l'injection de
défauts, la traçabilité et la CI sont implémentés et testés en simulation.

- **Validé (SIL)** : cœur Modbus RTU (FC03/FC06), machine de sûreté, DUT hôte, transport virtuel,
  injections, oracle, traçabilité, campagnes, CI.
- **Non validé** : portage STM32/RS-485 physique (architecture préparée, stubs + guide disponibles).
  Ne pas annoncer de validation matérielle tant qu'aucun essai sur carte n'a été réalisé.

## Prérequis

- CMake ≥ 3.16
- Compilateur C++17 (GCC ≥ 11 / Clang ≥ 14 / MSVC 2019+)
- Python ≥ 3.9 avec `pytest` et `pyyaml` (`pip install pytest pyyaml`)

## Build (hôte)

```bash
# Windows (MSYS2/UCRT64)
# Terminal UCRT64
cmake -G Ninja -S . -B build
cmake --build build --parallel

# Tests
cd build
./rtufaultlab_tests.exe
```

```bash
# Linux
cmake -G "Unix Makefiles" -S . -B build
cmake --build build
```

## Tests

```bash
cd build
# Windows (les DLL MinGW doivent être accessibles : préfixer le PATH avec C:\msys64\ucrt64\bin
# si le binaire est compilé avec l'environnement UCRT64)
./rtufaultlab_tests.exe

# Linux
./rtufaultlab_tests
```

Python :

```bash
pip install pytest pyyaml
python -m pytest tests/integration tests/unit/test_bench_runner.py -q
```

## Campagnes

```bash
python bench/runner/cli.py bench/campaigns/smoke.yaml --dut build/rtufaultlab_dut_host.exe --reports bench/reports
```

Sur Windows, lancer depuis un shell dont le `PATH` contient le répertoire des DLL du
compilateur (ex. `C:\msys64\ucrt64\bin`). En cas d'échec de démarrage du DUT, le runner
affiche un diagnostic explicite.

## Démonstration

```bash
python bench/demo.py
```

Affiche une campagne nominale (smoke) puis une campagne avec défauts (robustness).

## Documentation

Voir `docs/README.md` pour le plan en phases et `docs/SPEC.md` pour la spécification.

## Périmètre

- **Simulé (SIL)** : firmware hôte, transport virtuel, injection de défauts.
- **Matériel** : portage STM32 optionnel (jalon M5), non validé sans carte.

## Licence

MIT — voir `LICENSE`.
