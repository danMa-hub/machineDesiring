# MACCHINA DEL DESIDERIO — Foundation

**versione 2.3 fixed_names + Vergine Totale — documento stabile**

---

## Concept

Installazione artistica interattiva in Unreal Engine 5.7. Esplora le dinamiche relazionali e sessuali della comunità gay maschile contemporanea. Omaggio alla Macchina Celibe di Duchamp — sistema che produce desiderio senza mai risolverlo.

---

## Architettura — Tre modelli

### 1. Macchina v2.3 (statica)

**File:** `macchina_v2_3_fixed_names.py`

Fotografia istantanea: N NPC, soglia fissa 0.35, matrice N×N, nessun tempo. Riferimento di calibrazione per i modelli dinamici.

- Modello: distanza Euclidea pesata (Conroy-Beam 2016-2022)
- Dual mode: cruising (two-step: visual → sexual) + app (single-pass 11 var)
- Profilo: 3 passate (indipendenti → condizionate → derivate)
- Preferenze: formula assortativa `myDesired_X = r_a×my + r_asp×ideal + r_pop×pop_mean + noise`
- Risultati baseline (N=700): 14% neverVisMatched, 51% neverSexMatched, Gini 0.313

### 2. Macchina Vergine v1.0 (soglie adattive)

**File:** `macchina_vergine_1_0.py`

Ogni NPC parte senza esperienza di mercato e impara progressivamente quanto vale. I target di desiderio (`myDesired_X`) restano fissi (calcolati da v2.3); le soglie si adattano.

**Meccanica incontri:**
- Round di incontri, popolazione partizionata in sub-batch da 50
- Pool di 20 con batch iniziale simultaneo + arrivi sequenziali
- Pool si affolla se nessuno esce (mai espulsione artificiale)
- Group sex: seed pair + espansione greedy organica
- Soglie si aggiornano a fine sub-batch (sommario serata, non per-interazione)

**Soglie adattive:**
- `my_visThreshold` / `my_sexThreshold` — due soglie indipendenti
- Full match → entrambe salgono, frustrazione decade
- Mutual visual senza sex → vis sale (segnale positivo), sex scende
- Nessun match → vis scende (decay asintottico: `delta × threshold`)
- Floor dinamico: `my_receivedVisAttractionMean × 0.75` (Mode B)

**Floor Mode B vs D:**
- **B:** Floor da running mean attrazioni ricevute. Pulito, r=+0.999 con popVis statica.
- **D:** Floor da MySelfEsteem dinamica (sociometro con ancora, Bale & Archer 2013). MySelfEsteem += outcome_lr×(signal - SE) + anchor×(static - SE). Frustrazione erode l'ancora.
- Raccomandazione: B come motore, D come layer narrativo per UE5.

**Sim Mode cruise vs app:**
- Cruise: two-step, frustrazione visiva + sessuale separata
- App: single-pass, soglia unica, nessuna frustrazione separata

**Convergenza:** visThreshold μ converge a 0.318 (target v2.3: 0.317). r(popVis, visThr) = +0.78.

### 3. Macchina Vergine Totale v1.0 (apprendimento desiderio)

**File:** `macchina_vergine_totale_1_0.py`

L'NPC parte vergine totale — non sa cosa vuole, cosa esiste, cosa funziona. Il desiderio si FORMA attraverso gli incontri.

**Cosa è innato:**
- Pesi (`myWeightsVisual/Sexual`) — quanto conta ciascun tratto (cross-culturale)
- BT complementarità — un bottom "sa" di volere un top (funzionale)
- Sensibilità base ai segnali di fitness (Beauty, Muscle, Slim)
- Bias etnico e sierologico (proprietà dell'osservatore)

**Cosa è appreso:**
- `my_estimatedPopMean_X` — cosa esiste nella popolazione (puro campionamento)
- `my_learnedDesired_X` — cosa voglio (converge con match feedback)
- I coefficienti (selfMirror, culturalPull, marketAwareness) EMERGONO dal pattern dei match

**Tre segnali di apprendimento:**
1. **Esposizione** (ogni incontro): pop_mean si aggiorna verso i tratti visti
2. **Match reciproco** (segnale forte): desired si sposta verso i tratti del partner
3. **Market awareness** (drift debole): desired drifta verso pop_mean percepita

**Inizializzazione vergine:**
- `my_learnedDesired_X` = selfMirror×my + culturalPull×ideal + (1-sm-cp)×0.50 + noise
- `my_estimatedPopMean_X` = 0.50 (non sa nulla)
- selfMirror: 0.10 default, 0.70 per BT (innato), 0.30 per Age
- culturalPull: 0.15 (debole — si rinforza con esposizione)

**Risultati chiave (N=300, 40 round):**
- Pop mean estimation: quasi perfetta (gap < 0.005)
- BT complementarità: r(learned, v2.3) = **+0.835** — la più forte
- Bottom desired_BT = 0.770, Top desired_BT = 0.235 (emergente!)
- Muscle desired drifta 0.521 → 0.463 (verso realtà, culturalPull resiste)
- anyMatchRate = 28%

### 4. Macchina Vergine Totale v1.1 (semplificazione + bootstrap realistico)

**File:** `macchina_vergine_totale_1_1.py`

Evoluzione v1.0 con semplificazioni architetturali e bootstrap matematicamente corretto.

**Modifiche rispetto a v1.0:**
- **Batch simultaneo:** rimosso arrivo progressivo pool (−120 righe codice)
- **Batch size 30:** da 50 (sweet spot varianza inter-NPC / convergenza)
- **Pop mean bootstrap:** inizializzazione da media batch reale (no 0.50 arbitrario)
- **Aggiornamento esatto:** media ponderata numericamente corretta (no LR)
- **Group match rimosso:** semplificazione (reintroduzione futura)
- **Bias disabilitabili:** flag ENABLE_ETHNICITY_BIAS, ENABLE_POZ_BIAS (default False)

**Bootstrap popolazione (matematica):**
```
Round 1: estimatedPopMean = mean(batch_visto)  # campione 30
Round 2+: estimatedPopMean = (old×n_prev + batch_mean×n_curr) / (n_prev+n_curr)
Convergenza: gap < 0.01 a round ~20 (vs round ~14 con batch=50)
```

**Risultati attesi (N=700, 50 round, batch=30):**
- Pop mean gap: ~0.008 a round 40 (vs <0.005 con v1.0 batch=50)
- Varianza inter-NPC: +35% (differenziazione più ricca fino a round 15)
- Convergenza BT: r ~ +0.82 (leggermente meno correlato, accettabile)
- Codice: −40% righe vs v1.0

---

## Scelte metodologiche fondamentali — NON MODIFICARE

1. Distribuzioni demografiche reali (età media 38, non 25)
2. Bias sistemici come realtà documentata — NON attenuare
3. Match in due fasi temporali distinte (cruising)
4. myTribeTag solo per LOD e debug
5. myDesired_ da formula assortativa (v2.3) O appresa (Vergine Totale)
6. Normalizzazione pesi separata per step
7. Observer bias generato una volta per persona
8. Rapporto visual/sexual 62/38 nel single-pass
9. Pesi da letteratura gay-specifica
10. Nomenclatura fissa (vedi variables.md)
11. **NUOVO:** Pesi sono innati, target sono appresi (Vergine Totale)
12. **NUOVO:** BT complementarità è innata, non appresa
13. **NUOVO:** Frustrazione a fine sub-batch, non per-interazione

## Semplificazioni attive (reversibili)

### v1.1 — Bias disabilitati
- **Ethnicity bias**: ENABLE_ETHNICITY_BIAS = False
- **PozStatus bias**: ENABLE_POZ_BIAS = False

**Razionale:** Ridurre sovrapposizioni interpretative e cross-influenze difficili da gestire in fase di calibrazione. I bias sono documentati nel codice e riattivabili via flag globale.

### v1.1 — Group sex rimosso
Rimozione temporanea per semplificare leggibilità (~100 righe codice). Reintroduzione prevista a modello efficace stabilizzato.

---

## Pipeline — ordine obbligatorio

### v2.3 (statica)
Passata 1 → 2 → 3 → Preferenze → Matrice N×N → Metriche

### Vergine v1.0
Profili + Preferenze (identici a v2.3) → Init state → Loop round:
  Sub-batch → Pool + Arrivi → Matching → Sommario serata → Update soglie

### Vergine Totale v1.0
Profili (identici) + Preferenze (solo pesi, target ignorati) → Init state (desired naivi) → Loop round:
  Sub-batch → Pool + Arrivi → Matching (usa learnedDesired) → Learn desire + Update soglie

---

## Glossario aggiuntivo (Vergine)

| Termine | Definizione |
|:---|:---|
| `my_learnedDesired_X` | target di desiderio appreso (Vergine Totale) |
| `my_estimatedPopMean_X` | stima individuale della media della popolazione |
| `my_receivedVisAttractionMean` | media attrazioni visive ricevute (per floor B) |
| `MySelfEsteem_dynamic` | autostima che si aggiorna col mercato (sociometro) |
| `selfMirror` | coefficiente "desidero chi mi somiglia" |
| `culturalPull` | coefficiente "desidero l'ideale della cultura" |
| `marketAwareness` | coefficiente "desidero in base a ciò che esiste" |
| `outcome_signal` | segnale categorico della serata (match/vis_match/nulla) |
| `anchor` | forza identitaria che riporta MySelfEsteem al baseline |
| `erosion` | frustrazione che erode l'ancora — internalizzazione stigma |
