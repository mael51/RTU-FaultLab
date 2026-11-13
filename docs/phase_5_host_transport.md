# Phase 5 - Abstraction de transport et DUT hôte (SIL)

**Statut** : ✅ Validée
**Objectif** : Implémenter l'interface de transport, l'adapter hôte (loopback) et le processus `rtufaultlab_dut_host` échangeant des enveloppes hex sur stdin/stdout (jalon M2).

```mermaid
flowchart LR
    PC[PC / runner] -->|REQ hex| DUT[rtufaultlab_dut_host]
    DUT -->|RESP hex| PC
    DUT --> P[Parser]
    P --> H[Handler]
    H --> R[RegisterMap]
```

---

## 🧠 Notions clés

- **IByteTransport** : interface commune (loopback, série, RS-485).
- **Enveloppe IPC** : `REQ <hex>` / `RESP <hex>` par ligne, octets bruts réinjectés dans le parser.
- **Logs diagnostic** : uniquement sur stderr.
- **SIL** : Software-In-the-Loop, exécutable sans matériel.

---

## 🛠️ Étapes à réaliser

1. Interface `IByteTransport`.
2. Transport loopback (in-memory, pour tests unitaires).
3. Transport série réel (SerialTransport C++ + serial_transport.py Python).
4. DUT host process : lecture stdin, décodage hex, parser, réponse hex sur stdout.
5. Gestion des silences inter-trame/inter-caractère avec horloge simulée.
6. Tests IPC : enveloppes valides, logs sur stderr, erreurs de format.

---

## ✅ Checklist de validation

- [ ] Processus DUT hôte compile et démarre
- [ ] Enveloppe REQ/RESP hex fonctionne
- [ ] Logs uniquement sur stderr
- [ ] Parser reçoit octets bruts
- [ ] Transport loopback testable
- [ ] Tests verts

---

## 📁 Fichiers à créer/modifier

| Fichier | Action |
|---|---|
| `firmware/core/interfaces/IByteTransport.hpp` | Créer |
| `firmware/host/transport_loopback.hpp/.cpp` | Créer |
| `firmware/host/dut_host_main.cpp` | Créer |
| `tests/unit/test_ipc.cpp` | Créer |