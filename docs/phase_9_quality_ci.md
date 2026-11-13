# Phase 9 - Qualité, sanitizers et CI GitHub Actions

**Statut** : ✅ Validée
**Objectif** : Mettre en place la qualité continue : clang-format, warnings, clang-tidy/cppcheck, sanitizers (ASan/UBSan), GitHub Actions (jalon M4).

```mermaid
flowchart LR
    PR[Pull Request] --> CI[GitHub Actions]
    CI --> B[Build]
    CI --> T[Tests C++]
    CI --> P[Tests Python]
    CI --> F[Format check]
    CI --> S[Sanitizers (Linux)]
    CI --> G[Gate: bloque si échec]
```

---

## 🧠 Notions clés

- **Sanitizers** : détection accès hors limites, UB.
- **Analyse statique** : clang-tidy, cppcheck.
- **CI** : build + tests + format + analyse sur PR/main.

---

## 🛠️ Étapes à réaliser

1. `.clang-format` et script de vérification.
2. CMake options sanitizers.
3. Workflow GitHub Actions (build host, tests, Python).
4. Rapport d'analyse statique.
5. Gate : PR bloquée si échec.

---

## ✅ Checklist de validation

- [ ] clang-format vérifié en CI
- [ ] Warnings traités comme erreurs (option)
- [ ] Sanitizers passent
- [ ] Analyse statique intégrée
- [ ] CI verte sur PR

---

## 📁 Fichiers à créer/modifier

| Fichier | Action |
|---|---|
| `.clang-format` | Créer |
| `.github/workflows/ci.yml` | Créer |
| `CMakeLists.txt` | Modifier (sanitizers) |