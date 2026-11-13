# RTU-FaultLab

Banc automatisé de validation firmware avec injection de défauts Modbus RTU / RS-485.

> Firmware C/C++ portable • banc de test Python/Pytest • SIL sans matériel

## Statut

Projet complet (phases 0–11). Firmware C++ portable, banc Python, injection de défauts déterministe, traçabilité et CI.

## Fonctionnalités

- Esclave Modbus RTU : FC03 (Read Holding Registers), FC06 (Write Single Register).
- Contrôle de deux sorties logiques, état sûr, timeout heartbeat (1000 ms).
- Transport virtuel PC + injecteur de défauts déterministe (drop, delay, duplicate, corrupt_crc, truncate).
- Firmware C++17 portable + adapter hôte (loopback, stdin/stdout) + architecture adapter STM32.
- Pytest, CLI, scénarios YAML, rapports JUnit XML / JSON / HTML, traçabilité.
- CI GitHub Actions (build, tests, format, sanitizers Linux, smoke campaign).

## Build (hôte)

```bash
# Windows (MSYS2/MinGW)
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
cmake -G "MSYS Makefiles" -S . -B build
cmake --build build
```

```bash
# Linux
cmake -G "Unix Makefiles" -S . -B build
cmake --build build
```

## Tests

```bash
cd build
# Windows
$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
.\rtufaultlab_tests.exe

# Linux
./rtufaultlab_tests
```

Python :

```bash
pip install pytest pyyaml
python -m pytest tests/integration -q
```

## Campagnes

```bash
python bench/runner/cli.py bench/campaigns/smoke.yaml --dut build/rtufaultlab_dut_host.exe --reports bench/reports
```

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

MIT (à compléter).