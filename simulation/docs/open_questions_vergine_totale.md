# MACCHINA DEL DESIDERIO — Open Questions

**versione 2.3 fixed_names + Vergine Totale — aggiornare ad ogni sessione**

---

## Verifiche risolte in v2.0–v2.3

Vedi file precedente per la lista completa. Tutti i TODO 1-13, BUG 1-5, FIX 1-11, RENAME-1 risolti.

---

## Verifiche risolte in v1.1 (sessione corrente)

| ID | Descrizione | Risoluzione |
|:---|:---|:---|
| DESIGN-residuo-0.5 | Componente residuo inizializzato a 0.50 arbitrario | **Risolto v1.1.** Rimosso 0.50, inizializzazione da media batch reale. |
| DESIGN-bootstrap-pop | estimatedPopMean inizializzava a 0.50 (ignoto) | **Risolto v1.1.** Bootstrap da batch: round 1 = mean(batch), round 2+ = media ponderata esatta. |
| SIMPLIFY-pool | Arrivo progressivo pool (120 righe) | **Risolto v1.1.** Batch simultaneo completo, codice −40%. |
| SIMPLIFY-group | Group sex complessità (~100 righe) | **Risolto v1.1.** Rimosso temporaneamente, reintroduzione futura. |
| SIMPLIFY-bias | Bias Ethnicity/Poz cross-influenze | **Risolto v1.1.** Flag reversibili ENABLE_*_BIAS = False. |
| OPTIMIZE-batch | Batch size 50 convergenza troppo rapida | **Risolto v1.1.** Batch = 30 (sweet spot varianza +35%, convergenza round 20-25). |

| ID | Descrizione | Risoluzione |
|:---|:---|:---|
| TODO-Vergine-1 | my_visFrustration non differenziata | **Risolto.** Floor dinamico da `my_receivedVisAttractionMean × 0.75` (Mode B). Frustrazione ora ha σ=0.87 (era 0.028). r(popVis, visFrust) = −0.77. |
| BUG-pool | Pool espelleva NPC artificialmente quando pieno | **Risolto.** Almeno 1 newcomer per ciclo, pool si affolla naturalmente. |
| BUG-4 | pop_neverVisDesired calcolata su uscenti | **Risolto fixed_names.** `in_vis = vis_pass.sum(axis=0)`. |
| DESIGN-floor-D | MySelfEsteem crollava da 0.45→0.19 | **Risolto.** Sociometro con ancora identitaria + erosione da frustrazione. Δ ora = −0.13, r(stat,dyn) = +0.76. |

---

## Verifiche pendenti

### [TODO-1] Versatile penalizzato nel match BottomTop

**Status:** Confermato nella Vergine. Versatili: sexFrust=1.98, 0 fullMatch in 25 round (profiling). Accettabile come realistico (Moskowitz 2008).

### [TODO-3] σ — calibrazione empirica

**Status:** σ fisso confermato come scelta corretta. Non varia nella Vergine.

### [TODO-VT-1] Vergine Totale — Beauty desired non converge a v2.3

**Problema:** `my_learnedDesired_Beauty` μ=0.504 vs v2.3 μ=0.590. La componente culturale (`INITIAL_CULTURAL_STRENGTH=0.15`) è troppo debole per riprodurre il livello aspirazionale di v2.3. Possibile fix: aumentare INITIAL_CULTURAL_STRENGTH o aggiungere rinforzo culturale dagli incontri (chi vede molti belli innalza l'ideale).

### [TODO-VT-2] Vergine Totale — Age desired troppo alto

**Problema:** `my_learnedDesired_Age` μ=0.338 vs v2.3 μ=0.216. Il youth bias è più debole perché appreso dagli incontri (la popolazione ha età media 38) invece di culturalmente imposto. Narrativamente interessante: senza cultura del giovanilismo, il desiderio è meno ageista.

### [TODO-VT-3] r(learned, v2.3) basso per Muscle e Beauty

**Problema:** r=+0.09 (Muscle), r=+0.05 (Beauty). I target aspirazionali non differenziano tra individui perché il culturalPull è uguale per tutti e il selfMirror è debole. In v2.3 il r_assort crea differenziazione. Fix: aumentare selfMirror o far emergere la differenziazione dai match.

### [TODO-APP-1] App matchRate troppo alto

**Problema:** 82% anyMatchRate vs target ~50%. `LR_FULL_MATCH_UP=0.025` è troppo generoso nell'app dove i match sono frequenti. Fix: ridurre a ~0.010 per modalità app.

---

## Decisioni aperte

### [APERTA-11] Riattivazione bias

Ethnicity e PozStatus bias disabilitati per semplicità (v1.1). 
Quando riattivare:
- Dopo stabilizzazione apprendimento desiderio
- Per confronto empirico bias vs no-bias
- Per narrativa UE5 (rappresentazione stigma)

### [APERTA-12] Group sex

Rimosso temporaneamente (~100 righe codice, v1.1). 
Reintroduzione quando:
- Pool dynamics consolidato
- Convergenza soglie verificata
- UE5 richiede feature per narrativa

### [APERTA-13] Modello aspirazionale (FASE 2)

Proposta: sostituire inizializzazione desiderio con:
```
desired = selfMirror×my + ASPIRATION_INNATE×better_than_self
better_than_self = my + 0.65×(1.0 - my)
```
Letteratura supporto:
- Buston & Emlen 2003: tutti overestimate mate value (+0.23 bias)
- Todd et al. 2007: aspirazione scende con fallimenti
- Regan 2008: 87% adolescenti pre-relazionali desidera "meglio di sé"

Da verificare:
- Coefficiente aspirazionale emerge come CULTURAL_IDEAL a livello popolazione?
- Convergenza dopo fallimenti ripetuti produce pattern assortativo v2.3?
- Calibrazione decay rate aspirazione (LR ~ 0.02-0.05?)



Mode B (receivedVisMean) produce metriche identiche a D ma è più semplice. Mode D (MySelfEsteem sociometro) è più utile per UE5. **Raccomandazione:** B come motore di calcolo, D come layer narrativo sovrapposto.

### [APERTA-9] Vergine vs Vergine Totale

Due modelli distinti con scopi diversi:
- **Vergine v1.0:** soglie adattive, target fissi (da v2.3). Converge al baseline v2.3.
- **Vergine Totale v1.0:** soglie + target appresi. Mostra come il desiderio si FORMA.
Entrambi vanno mantenuti. La Vergine è il modello di produzione per UE5; la Totale è il modello esplorativo/narrativo.

### [APERTA-10] Innato vs appreso — calibrazione

Tre parametri chiave nella Vergine Totale da calibrare:
- `INITIAL_CULTURAL_STRENGTH` (0.15) — quanto l'ideale culturale influenza prima degli incontri
- `INITIAL_SELF_MIRROR` (0.10) — quanto il self-reference è innato
- `LR_MATCH_DESIRED` (0.12) — quanto un match sposta il desiderio

---

## Agenda prossima sessione

**Priorità alta:**
1. **FASE 2 — Modello aspirazionale** (discussione design)
   - Sostituire selfMirror+culturalPull con ASPIRATION_INNATE
   - Decay aspirazione da fallimenti → emergenza assortativo
   - Verifica emergenza CULTURAL_IDEAL come fenomeno popolazione
2. Verifica empirica v1.1 (N=700, 50 round, batch=30)
   - Confronto convergenza vs v1.0
   - Verifica varianza inter-NPC aumentata
   - Gap pop_mean < 0.01 a round 20?

**Priorità media:**
3. Nuovi segnali apprendimento (dopo FASE 2 stabilizzata)
   - Gradimento ricevuto non contraccambiato (LR=0.03)
   - Desiderio espresso respinto (LR=−0.02)
   - Vis match senza sex (differenziato vis/sex)
   - Isolamento sociale (my_isolationFrustration)
4. Calibrazione Vergine Totale v1.0 residua (se ancora rilevante)
   - TODO-VT-1/2/3 (Beauty, Age, r_correlation bassi)

**Priorità bassa:**
5. Traduzione v1.1 in C++ per UE5 (dopo modello aspirazionale)
6. Profiling archetipi Vergine Totale
7. Report visivo PNG per v1.1

