# Session Log — Macchina del Desiderio

Log cronologico delle sessioni di lavoro. Aggiornato a ogni chiusura.

---

## 2026-05-17 — sessione 2

**Obiettivo:** Impostare protocollo di lavoro e workflow sessioni

**Fatto:**
- Aggiunto protocollo apertura/chiusura sessione in CLAUDE.md
- Creato `.claude/session_log.md`
- Creato `.claude/out_of_credits_log.md` con template manuale
- Creati due collegamenti sul Desktop (log crediti + VS Code progetto)

**File archiviati:** nessuno

**Prossimo:** UE5-1 — implementare `BP_Execute*` in `BP_NPC_AIController`

**Stato roadmap:** invariato (UE5-1 ⏳, EP01-1 ⏳, PY-1 ○)

---

## 2026-05-17 — sessione 1

**Obiettivo:** Setup iniziale del repo e organizzazione file

**Fatto:**
- Creata struttura cartelle `assets/environments/ep01-beach/` (pineta, scogliere, zona-cruising, area-04..07)
- Creata struttura `assets/characters/`, `assets/props/`
- Creata struttura `simulation/` e `simulation/docs/`
- Spostato `testSetCruising.blend` + `.blend1` → `assets/environments/ep01-beach/`
- Eliminato `assetsxBlender - collegamento.lnk`
- Spostati da `E:\downloadWin\files\` i modelli Python e tutta la documentazione nelle cartelle corrette
- Aggiunto protocollo sessione a CLAUDE.md
- Creato questo session_log.md

**File archiviati:**
- `macchina_base_v2_4.py` → `simulation/`
- `macchina_vergine_totale_1_1.py` → `simulation/`
- 9 file `.md` documentazione → `simulation/docs/`
- `testSetCruising.blend` / `.blend1` → `assets/environments/ep01-beach/`

**Prossimo:** UE5-1 — implementare `BP_Execute*` in `BP_NPC_AIController` (movimento reale CharacterMover/NavMesh)

**Stato roadmap:**
- UE5-1 ⏳ (priorità alta)
- EP01-1 ⏳ (definire micro-aree ep01-beach)
- PY-1 ○ (fix import rotto Vergine Totale)
