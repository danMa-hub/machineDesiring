# simulation.md — Modelli Python
# Macchina del Desiderio

---

## Tre modelli

| Modello | File | Scopo |
|---------|------|-------|
| v2.4 statico | `simulation/macchina_base_v2_4.py` | Baseline calibrazione, fotografia istantanea |
| Vergine v1.0 | `simulation/macchina_vergine_1_1.py` | Soglie adattive, modello di produzione per UE5 |
| Vergine Totale v1.1 | `simulation/macchina_vergine_totale_1_1.py` | Desiderio appreso, modello esplorativo/narrativo |

**Baseline N=700, seed=42.**

---

## Formula attrazione — Distanza Euclidea Pesata

Fonte: Conroy-Beam (2016-2022), N=14487, 45 paesi. Campione eterosessuale — math
valido universalmente, pesi da letteratura gay-specifica.

```
d = sqrt( Σ w_i × (myDesired_i - trait_i)² )
attraction = exp( -d² / (2σ²) )
```

σ_cruising = 0.13 (7 vis + 4 sex var)
σ_app      = 0.15 (11 var insieme)
I σ diversi compensano la dimensionalità — scelta documentata, non bug.

---

## Dual mode

### MODE_CRUISING — incontro fisico (cruising, bar, darkroom)

```
Fase 1 — visual (7 var + 2 moltiplicatori):
  myOut_visAttraction(A→B) ≥ threshold
  AND myOut_visAttraction(B→A) ≥ threshold
  → pair_visMatched = true

Fase 2 — sexual (4 var), solo se pair_visMatched:
  myOut_sexAttraction(A→B) ≥ threshold
  AND myOut_sexAttraction(B→A) ≥ threshold
  → pair_visSexMatched = true
```

Frustrazione sessuale strutturale emergente: ~88% dei pair_visMatched non arriva a
pair_visSexMatched.

### MODE_APP — dating app (Grindr, Scruff)

```
Calcolo unico: 11 variabili + bias post-distanza. σ=0.15.
Pesi raw dalla letteratura normalizzati a somma 1.
Dealbreaker BT: bt_distance > 0.60 → ×0.15
myOut_attraction_app(A→B) ≥ threshold AND (B→A) ≥ threshold → pair_fullMatched
```

---

## Variabili nel match

### Visual cruising (7 var + 2 moltiplicatori)

| Variabile | Peso base | Tipo | Fonte |
|-----------|-----------|------|-------|
| MyMuscle | 0.28 | aspirazionale | Swami & Tovée 2008 |
| MyBeauty | 0.23 | aspirazionale | Levesque & Vichesky 2006 |
| MySlim | 0.15 | aspirazionale | Swami & Tovée 2008 |
| MyMasculinity | 0.15 | assortativo | Bailey 1997 |
| MyAge → myAgeNorm | 0.07 | assort.+youth bias | Antfolk 2017 |
| MyHair | 0.05 | bipolare | Martins 2008 |
| MyHeight → myHeightNorm | 0.04 | aspirazionale | Valentova 2014 |
| MyEthnicity → myEthBias | molt. post-dist. | — | OkCupid 2009 |
| MyPozStatus → myPozBias | molt. post-dist. | — | CDC 2022 |

### Sexual cruising (4 var)

| Variabile | Peso base | Modulazione | Fonte |
|-----------|-----------|-------------|-------|
| MyBottomTop | 0.40 | ×(1+extremity×0.60) | Moskowitz 2008 |
| MySubDom | 0.28 | — | Bailey 1997 |
| MyKinkLevel | 0.17 | ×(1+KL×0.80) | Parsons 2010 |
| MyPenisSize | 0.15 | ×(1+bottomness×0.80) | GMFA 2017 |

### App single-pass (11 var, pesi normalizzati)

| Variabile | Peso norm. |
|-----------|-----------|
| BottomTop | 0.203 |
| Muscle | 0.142 |
| SubDom | 0.142 |
| Beauty | 0.117 |
| KinkLevel | 0.086 |
| Slim / Masc / PenisSize | 0.076 |
| Age | 0.036 |
| Hair | 0.025 |
| Height | 0.020 |

---

## Pipeline generazione profili

**Passata 1 — indipendenti:**
MyAge, MyHeight, MyEthnicity, hair_base, beauty_base, bodymod_base,
bodydisplay_base, MyPolish, MyQueerness, MyKinkLevel, MyOpenClosed,
MyRelDepth, MyPenisSize

**Passata 2 — condizionate:**
myAgeNorm, myHeightNorm, MyHair, MyBodyMod, MyBottomTop, MyMuscle,
MySlim, MyBeauty, MyMasculinity, MyBodyDisplay, MySexExtremity,
MySexActDepth, MySubDom, MyPozStatus, MySafetyPractice

**Passata 3 — derivate:**
MySelfEsteem, myTribeTag, myKinkTypes

**Preferenze (MatingPreferences):**
```
Step 1: myDesired_X per tutte le var in ASSORT_COEFFS
  complementare (r_self<0): myDesired = |r_self|×(1-my) + (1-|r_self|)×pop_mean
  aspirazionale (r_ideal>0): myDesired = r_self×my + r_ideal×ideal + r_pop×pop_mean
  assortativo puro:          myDesired = r_self×my + (1-r_self)×pop_mean
  + noise gaussiano individuale

Step 2: myDesired_PenisSize (BT-condizionale, fuori ASSORT_COEFFS)
Step 3: myWeightsVisual / myWeightsSexual / myWeightsSingle (BASE + noise 30-40%)
Step 4: myEthBias / myPozBias (una volta per persona)
```

---

## Modello v2.4 statico — risultati baseline N=700

```
pop_visAttractionPolarization (Gini)  : 0.313
pairs_visCompatibilityRate_cruise     : ~8%
pop_neverVisMatched_cruise            : 14%
pop_neverSexMatched_cruise            : 51%
pairs_bottomTopIncompatibilityShare   : ~35%
```

---

## Vergine v1.1 — soglie adattive

Target fissi (da v2.4). Soglie adattive per-NPC.

**Meccanica:**
- Round di incontri, batch = 30 NPC
- Full match → entrambe le soglie salgono, frustrazione decade
- Mutual visual senza sex → vis sale, sex scende
- Nessun match → vis scende (decay: delta × threshold)
- Floor dinamico: `my_receivedVisAttractionMean × 0.75` (Mode B)

**Convergenza:** visThreshold μ → 0.318 (target v2.4: 0.317). r(popVis, visThr) = +0.78.
r(popVis, visFrust) = −0.77. Modello di produzione per UE5.

**Floor Mode B vs D:**
- B: floor da running mean attrazioni ricevute. Più semplice, r=+0.999 con popVis statica.
- D: floor da MySelfEsteem sociometro (Bale & Archer 2013). Più utile come layer narrativo UE5.
- Raccomandazione: B come motore, D come layer narrativo sovrapposto.

---

## Vergine Totale v1.1 — desiderio appreso

Cosa è **innato:**
- Pesi (myWeightsVisual/Sexual) — cross-culturale
- BT complementarità — funzionale
- Sensibilità base fitness (Beauty, Muscle, Slim)
- Bias etnico e sierologico

Cosa è **appreso:**
- `my_estimatedPopMean_X` — cosa esiste (puro campionamento)
- `my_learnedDesired_X` — cosa voglio (converge con match feedback)

**Tre segnali apprendimento:**
1. Esposizione (ogni incontro) → pop_mean si aggiorna verso tratti visti
2. Match reciproco (segnale forte) → desired si sposta verso tratti partner
3. Market awareness (drift debole) → desired drifta verso pop_mean percepita

**Parametri chiave:**
```
INITIAL_CULTURAL_STRENGTH = 0.15
INITIAL_SELF_MIRROR       = 0.10
LR_MATCH_DESIRED          = 0.12
batch = 30, round convergenza ~20-25
```

**Import rotto (bug aperto):** importa da `macchina_v2_3_fixed_names` che non esiste.
Fix: aggiornare import a `macchina_base_v2_4`.
