# Guide de portage matériel (STM32 / RS-485)

## Câblage basse tension

- UART STM32 (3.3V) ↔ transceiver RS-485 (MAX485 ou équivalent).
- DE/RE contrôlés par un GPIO pour le demi-duplex.
- Alimentation 3.3V uniquement ; pas de charge dangereuse.

## Configuration STM32

- UART : 19200 bauds, 8 bits, parité paire, 1 stop (8E1).
- FreeRTOS : une tâche pour le DUT (parser + sécurité).
- Horloge : SysTick 1 ms.

## Build cross

```bash
cmake -G "MSYS Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE="firmware/stm32/toolchain-arm.cmake" \
  -DCROSS_COMPILE=ON -S . -B build_stm32
cmake --build build_stm32
```

## Limites de validation

- Portage optionnel (jalon M5).
- Stubs à compléter avec HAL.
- Renode possible après stabilisation hôte.