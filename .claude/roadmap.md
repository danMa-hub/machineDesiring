# roadmap.md — TODO unificati
# Macchina del Desiderio — aggiornare a ogni sessione

---

## UE5 — priorità alta

| ID | Task | Stato |
|----|------|-------|
| UE5-1 | `BP_Execute*` in `BP_NPC_AIController` — movimento reale CharacterMover/NavMesh | ⏳ |
| UE5-2 | `U_matingLogic` — Branch A/B, SexBlocked | ○ |
| UE5-3 | Stato `Latency` — cooldown post-mating | ○ |

## UE5 — priorità media

| ID | Task | Stato |
|----|------|-------|
| UE5-4 | `UW_runtimePanel` VIEW 2 — completare pannello runtime | ⏳ |
| UE5-5 | Plugin Mutable → `BP_ApplyMutableProfile` | ○ |

---

## Episodio 01 Beach — priorità alta

| ID | Task | Stato |
|----|------|-------|
| EP01-1 | Definire 7-8 micro-aree (concept + layout) | ⏳ |
| EP01-2 | Livello UE5 `ep01_beach` — terrain base | ○ |
| EP01-3 | Prima micro-area operativa (pineta) | ○ |

Dettaglio aree in @.claude/ep01-beach.md

---

## Python simulazione — priorità alta

| ID | Task | Stato |
|----|------|-------|
| PY-1 | Fix import rotto Vergine Totale (`macchina_v2_3_fixed_names` → `macchina_base_v2_4`) | ○ |
| PY-2 | FASE 2 Vergine Totale — modello aspirazionale | ○ |

## Python simulazione — priorità media

| ID | Task | Stato |
|----|------|-------|
| PY-3 | TODO-VT-1: Beauty desired non converge (μ=0.504 vs 0.590) | ○ |
| PY-4 | TODO-VT-2: Age desired troppo alto (μ=0.338 vs 0.216) | ○ |
| PY-5 | TODO-VT-3: r(learned, v2.4) basso per Muscle e Beauty | ○ |
| PY-6 | TODO-APP-1: matchRate app troppo alto (82% vs target ~50%) | ○ |
| PY-7 | Nuovi segnali apprendimento (gradimento non contraccambiato, rifiuto, isolamento) | ○ |
| PY-8 | Riattivare bias etnico/poz nella Vergine Totale | ○ |
| PY-9 | Reintrodurre group sex (rimosso in v1.1) | ○ |

## Python simulazione — priorità bassa

| ID | Task | Stato |
|----|------|-------|
| PY-10 | Calibrazione σ empirica (dati match rate app gay) | ○ |
| PY-11 | Profiling archetipi Vergine Totale | ○ |
| PY-12 | Report visivo PNG per v1.1 | ○ |

---

## Decisioni architetturali aperte

| ID | Decisione | Stato |
|----|-----------|-------|
| ARCH-1 | `myPop_globalAttraction` vs `myPop_appAttraction` — divergenza Python/C++ | ○ |
| ARCH-2 | Rename `mv_visual`→`vizAppeal` ecc. — abbandonato o da confermare | ○ |
| ARCH-3 | Modello aspirazionale FASE 2: `desired = selfMirror×my + ASPIRATION_INNATE×better_than_self` | ○ |
| ARCH-4 | Vergine vs Vergine Totale per UE5: Vergine è produzione, Totale è narrativa — confermato | ✓ |
| ARCH-5 | Floor Mode B (receivedVisMean) come motore + Mode D (MySelfEsteem) come layer UE5 | ✓ |

---

## Legenda
```
✓  completato
⏳ in corso
○  da fare
```
