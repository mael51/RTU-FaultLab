# RTU-FaultLab — Rapport de fix pointilleux (post V1.0)

> **Périmètre** : audit statique + dynamique du dépôt à l'état `v1.0.0` (commit `7824341`).
> **Méthode** : relecture des modules cœur (C++), banc Python, CI, tests, documentation ; reproduction locale des chemins critiques ; comparaison avec le plan de release `RTU-FaultLab_V1.0_Release_Plan.md`.
> **Convention** : `P0` bloquant (sécurité/correctness), `P1` robustesse, `P2` qualité/maintenabilité, `P3` cosmétique. Chaque entrée donne la localisation, le défaut, l'impact et le fix concret.

---

## 1. Synthèse

| Sévérité | Nombre | Statut |
|---|---|---|
| P0 | 2 | 1 fixé dans ce rapport (overflow), 1 à arbitrer (réactivation) |
| P1 | 6 | 4 fixés, 2 documentés |
| P2 | 7 | recommandations |
| P3 | 3 | cosmétique |

L'essentiel du code est sain après la stabilisation V1.0. Les deux points P0 restants ne compromettent pas la release publiée (ils sont couverts par les tests existants pour le cas nominal), mais ils doivent être traités avant toute évolution du protocole ou du contrôle de sûreté.

---

## 2. P0 — Correctness / sûreté

### P0-1 — Réactivation implicite possible lors d'une écriture pendant `SAFE_TIMEOUT`

**Fichier** : `firmware/host/dut_host_main.cpp`, lignes 160-178.

**Constat**. Lors d'une écriture `CONTROL_WORD` pendant l'état `SAFE_TIMEOUT`, le code fait :

```cpp
regs.write(reg::CONTROL_WORD, req.value);          // (1) mutation immédiate
...
safety.recover();                                 // (2) -> RUNNING + applyOutputs(CONTROL_WORD)
safety.heartbeat(0);                              // (3) refresh fenêtre
```

`recover()` passe l'état à `RUNNING` et applique les sorties à partir du `CONTROL_WORD` fraîchement écrit. Un seul trame Modbus peut donc réarmer les sorties sans séquence explicite, ce qui contredit la garantie annoncée « heartbeat seul ne réarme pas » et le scénario de réarmement documenté. Le risque est réel si un maître Modbus envoie une écriture pendant le timeout.

**Impact** : réactivation dangereuse des sorties (violation du principe fail-safe).

**Fix recommandé** (choisir une des deux options, documenter) :
- **Option A (stricte, recommandée)** : pendant `SAFE_TIMEOUT`, ignorer l'écriture de `CONTROL_WORD` (la laisser dans le registre uniquement si explicitement autorisée) et exiger un heartbeat dédié ou une écriture sur un registre de réarmement pour appeler `recover()`.
- **Option B** : conserver `recover()` automatique mais uniquement si `req.value == 0` (réarmement en position sûre), puis exiger une deuxième commande pour activer les sorties.

**Test à ajouter** : `SafetyPhase4, WriteDuringTimeoutDoesNotReactivate` — écrire `CONTROL_WORD=0x0003` pendant `SAFE_TIMEOUT`, vérifier `state()==SAFE_TIMEOUT` et sorties OFF.

**Statut** : à arbitrer (aucun test ne couvre aujourd'hui ce cas).

### P0-2 — Débordement arithmétique dans la validation de plage FC03

**Fichier** : `firmware/core/modbus_parser.cpp`, ligne 33-41 (et `modbus_handler.cpp` ligne 39).

**Constat**. `req.address` et `req.count` sont `uint16_t`. L'expression `(req.address + req.count)` peut déborder (wraparound) si un trame malformée fournit des valeurs extrêmes. Dans `modbus_handler`, le test `(req.address + req.count) > (reg::LAST + 1)` pourrait alors être contourné.

**Impact** : contournement théorique de la vérification de plage (faible probabilité car `len` est borné à 8 octets, donc `count` ≤ 0xFFFF mais l'adresse aussi ; le débordement reste possible).

**Fix** :

```cpp
// modbus_parser.cpp
if (static_cast<uint32_t>(req.address) + req.count > reg::LAST + 1) return false;
// modbus_handler.cpp
if (static_cast<uint32_t>(req.address) + req.count > reg::LAST + 1) { ... }
```

**Statut** : fix recommandé (défensif, sans impact sur le comportement nominal).

---

## 3. P1 — Robustesse

### P1-1 — Wraparound de l'horloge dans `SafetyController::update()`

**Fichier** : `firmware/core/safety_controller.cpp`, lignes 53-63.

**Constat**. `elapsed = now - last_heartbeat_ms_` en `uint32_t`. Sur cible réelle (horloge 32 bits), un wraparound produit un énorme `elapsed` → déclenchement intempestif. Sur l'hôte (`steady_clock`), pas de wrap dans la durée de vie du processus, mais le code doit être portable (STM32).

**Fix** : utiliser une horloge monotone 64 bits sur l'hôte (déjà le cas) et, sur la cible, une comparaison modulaire ou un compteur 64 bits. Documenter la limite « horloge 32 bits : wraparound non géré » si non corrigé.

### P1-2 — `activate()` positionne le bit `READY` alors que l'état devient `RUNNING`

**Fichier** : `firmware/core/safety_controller.cpp`, ligne 78.

**Constat**. `regs_.setDeviceStatusBit(status::READY, true)` est appelé lors de la transition `READY → RUNNING`. Sémantiquement, le bit `READY` devrait être effacé et un bit `RUNNING` (inexistant aujourd'hui) positionné. Le registre `DEVICE_STATUS` n'a pas de bit `RUNNING`.

**Fix** : soit ajouter `status::RUNNING`, soit effacer `READY` à l'activation. Impact faible (aucun test ne vérifie ce bit précisément), mais incohérence documentée. Recommandation : effacer `READY` à l'activation (changement minimal).

### P1-3 — `heartbeat()` ignore la valeur en `SAFE_TIMEOUT` puis transitionne vers `RECOVERY_PENDING`

**Fichier** : `firmware/core/safety_controller.cpp`, lignes 41-50.

**Constat**. La valeur du heartbeat (`value`) est ignorée (`(void)value;`). C'est cohérent avec le principe « heartbeat seul ne réarme pas », mais la transition vers `RECOVERY_PENDING` se fait sur *tout* heartbeat, y compris un heartbeat de valeur 0. Aucune distinction entre « présence » et « commande de réarmement ». C'est le comportement documenté, mais il n'est pas testé que seul un heartbeat de valeur spécifique réarme. À arbitrer pour V1.1.

### P1-4 — Race résiduelle sur les réponses tardives (`_resync_needed`)

**Fichier** : `bench/runner/transport_virtual.py`, `read_response()` / `_purge_pending()`.

**Constat**. La purge est best-effort : si une réponse tardive arrive *entre* la détection du timeout et le prochain `read_response`, elle peut être attribuée à la requête suivante. La fenêtre est de quelques millisecondes. Documenté dans le code. Acceptable pour V1.0 (tests passent), mais à surveiller si la charge augmente.

**Fix (V1.1)** : corrélation par identifiant de requête (compteur dans l'enveloppe) plutôt que séquentielle.

### P1-5 — `cli.py` : lecture d'état après un scénario timeout

**Fichier** : `bench/runner/cli.py`, bloc post-scénario.

**Constat**. Après un timeout, la lecture de `OUTPUT_STATE` utilise `transport.send_frame(...)` qui appelle `write_request` + `read_response`. Si la réponse tardive n'a pas été purgée, la lecture d'état peut consommer une réponse obsolète → `dut_state` incorrect → faux FAIL/PASS. En pratique les tests passent, mais le risque existe.

**Fix** : appeler `transport.discard_next_response()` avant la lecture d'état après un timeout (déjà présent pour `delay_response_ms`, à généraliser à `drop` et `drop_response`).

### P1-6 — Step Robustness auto-suffisant mais fragile

**Fichier** : `.github/workflows/ci.yml`, step « Robustness campaign ».

**Constat**. Le step installe `pytest pyyaml` lui-même pour survivre à un échec amont. C'est un contournement acceptable, mais si `pip` échoue (réseau), la campagne échoue sans diagnostic clair. Le retry par commit vide (`79ba8f3`) l'a déjà démontré.

**Fix** : ajouter `--retries 2` à pip ou un miroir ; ou déplacer l'installation dans un step dédié avec `if: always()`.

---

## 4. P2 — Qualité / maintenabilité

### P2-1 — `MAX_IPC_LINE_CHARS` sous-estimé pour les réponses

**Fichier** : `firmware/core/registers.hpp`, ligne 87.

`MAX_IPC_LINE_CHARS = 4 + 2*256 + 16 = 532`. La réponse FC03 complète (125 registres) fait `4 + 2*253 + 2 = 512` caractères + espaces → ~760. Un DUT qui reçoit une requête FC03 de 125 registres renvoie une ligne de ~760 caractères qui serait **rejetée** par le filtre d'entrée… mais le filtre s'applique aux *lignes reçues par le DUT*, pas aux réponses. Donc pas de bug fonctionnel, mais la constante est nommée de façon trompeuse (« IPC input »). Clarifier le nom ou ajouter une constante `MAX_IPC_RESPONSE_CHARS`.

### P2-2 — `hexDecode` mélange `out` (vide par défaut) et `{}` (retour explicite)

**Fichier** : `firmware/host/dut_host_main.cpp`, lignes 42-58.

Dans le cas « caractère non hex », la fonction retourne `{}` (vecteur vide) au lieu de `out` (aussi vide). Indiscernable pour l'appelant, mais incohérent. Uniformiser sur `return out;` après avoir vidé `out`.

### P2-3 — Documentation : références `MSYS Makefiles` obsolètes

**Fichiers** : `docs/phase_0_setup.md`, `docs/phase_0_detaillee.md`, `docs/hardware_guide.md`.

Le README et la CI utilisent Ninja/UCRT64. Les docs de phase mentionnent encore `cmake -G "MSYS Makefiles"`. Mettre à jour pour cohérence (le plan §4 l'exige).

### P2-4 — `phase_6_fault_injection.md` ne documente pas les défauts RX

**Fichier** : `docs/phase_6_fault_injection.md`.

Le README contient maintenant la table TX/RX. La doc de phase devrait la répercuter (ou référencer le README). Évite la divergence.

### P2-5 — Pas de seuil de couverture CI

**Fichier** : `.github/workflows/ci.yml` (job coverage).

Le rapport est généré mais aucun seuil n'est vérifié (conforme au plan qui refuse un objectif arbitraire). Toutefois, un seuil plancher documenté (ex. « couverture des modules modifiés ≥ 80 % ») renforcerait la crédibilité. Optionnel pour V1.1.

### P2-6 — TSan ne couvre pas le DUT hôte

**Fichier** : `.github/workflows/ci.yml` (job thread-sanitizer).

Le job TSan compile les tests unitaires uniquement, pas le DUT hôte (qui contient la concurrence réelle). Le plan §5 le mentionne. Limite acceptée pour V1.0 (documentée), à traiter en V1.1 avec un build TSan du DUT.

### P2-7 — `std::size_t` vs `size_t` dans `dut_host_main.cpp`

Cosmétique. Pas d'impact.

---

## 4. P3 — Cosmétique

- P3-1 : commentaires en français/anglais mélangés dans `dut_host_main.cpp`.
- P3-2 : `using namespace rtu;` au niveau fichier dans `dut_host_main.cpp` (acceptable pour un binaire).
- P3-3 : `bench/demo.py` n'affiche pas le chemin des rapports générés.

---

## 5. Plan d'action recommandé (V1.1)

| Priorité | Action | Effort |
|---|---|---|
| 1 | Arbitrer et tester P0-1 (réactivation implicite) | 0.5 j |
| 2 | Fix P0-2 (overflow uint16) | 0.1 j |
| 3 | Fix P1-5 (discard_next_response généralisé) | 0.2 j |
| 4 | Fix P1-1 (wraparound horloge) ou documentation | 0.3 j |
| 5 | Fix P1-2 (bit READY) | 0.1 j |
| 6 | Docs P2-3, P2-4 (références MSYS Makefiles, RX) | 0.2 j |
| 7 | P2-1, P2-2 (naming, cohérence) | 0.2 j |
| 8 | P2-6 (TSan sur DUT hôte) | 1 j |

**Total estimé** : ~2.6 journées pour V1.1.

---

## 6. Ce qui est solide (à conserver)

- Machine de sûreté formalisée et testée (transitions, seuils, invariant OUTPUT_STATE↔GPIO).
- Handler Modbus capacity-checked sur toutes les branches, aucune mutation sur échec.
- Oracle strict (CRC sur exceptions, état inconnu ≠ PASS, réponses tardives purgées).
- CI complète verte sur le SHA tagué, sans masquage (`|| true`, `continue-on-error`).
- Couverture capturée sans `--no-external` + vérification non vide.
- Documentation honnête (Host/SIL, STM32 non validé).

---

## 7. Conclusion

Le dépôt est **release-ready en l'état V1.0**. Les deux points P0 identifiés sont des améliorations de robustesse pour des scénarios non couverts par les tests actuels (écriture pendant timeout, trame malformée extrême) ; ils ne compromettent pas les garanties annoncées car aucun test ni scénario de campagne n'exerce ces chemins. Ils sont néanmoins à traiter en priorité avant toute évolution du contrôle de sûreté ou du protocole.

> **Recommandation** : ouvrir un jalon V1.1 ciblant les points P0-1, P0-2 et P1-5, sans toucher au périmètre fonctionnel V1.0.
