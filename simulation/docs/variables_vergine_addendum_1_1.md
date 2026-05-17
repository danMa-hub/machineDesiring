# VARIABLES ADDENDUM — Vergine Totale v1.1

Tutti i coefficienti, parametri, e risultati numerici del modello adattivo.

---

## PARAMETRI GENERALI

| Parametro | Valore | Note |
|:---|:---:|:---|
| **N_SUBJECTS** | 700 | Popolazione simulata |
| **N_ROUNDS** | 50 | Numero round |
| **SUBSAMPLE_SIZE** | 30 | Persone per batch |
| **RANDOM_SEED** | 42 | Riproducibilità |

---

## COEFFICIENTI ADATTIVI — Inizializzazione

### ASPIRATION

| Parametro | Valore | Note |
|:---|:---:|:---|
| **ASPIRATION_MEAN** | 0.65 | Media iniziale gaussiana |
| **ASPIRATION_SD** | 0.08 | Deviazione standard |
| **ASPIRATION_MIN** | 0.50 | Floor (dopo clip) |
| **ASPIRATION_MAX** | 0.80 | Ceiling iniziale |
| **ASPIRATION_CEILING** | 0.75 | Ceiling dopo successi |

**Formula inizializzazione:**
```python
aspiration = N(0.65, 0.08) clipped [0.50, 0.80]
```

**Distribuzione popolazione round 0:**
- μ = 0.65
- σ = 0.065 (ridotta da clip)
- Range tipico: [0.54, 0.76]

---

### SELFMIRROR

| Parametro | Valore | Note |
|:---|:---:|:---|
| **SELFMIRROR_MEAN** | 0.35 | Media iniziale gaussiana |
| **SELFMIRROR_SD** | 0.08 | Deviazione standard |
| **SELFMIRROR_MIN** | 0.20 | Floor |
| **SELFMIRROR_MAX** | 0.50 | Ceiling |

**Formula inizializzazione:**
```python
selfMirror = N(0.35, 0.08) clipped [0.20, 0.50]
```

**Distribuzione popolazione round 0:**
- μ = 0.35
- σ = 0.065
- Range tipico: [0.24, 0.46]

**Relazione con aspiration:**
- NON reciproci (1-aspiration)
- Indipendenti ma dinamica anticorrelata
- Correlazione emergente r ≈ −0.5

---

### CULTURALPULL

| Parametro | Valore | Note |
|:---|:---:|:---|
| **CULTURAL_PULL_INIT_BASE** | 0.15 | Base round 1 |
| **CULTURAL_PULL_INIT_MATCH_BONUS** | 0.10 | Bonus se match rate alto |

**Formula inizializzazione (round 1):**
```python
match_rate = my_fullMatchCount / batch_size
culturalPull = 0.15 + 0.10 × match_rate
```

**Range iniziale:** [0.15, 0.25]

**Distribuzione popolazione round 1:**
- μ ≈ 0.17
- σ ≈ 0.03
- Range: [0.15, 0.25]

---

### MARKETAWARENESS

| Parametro | Valore | Note |
|:---|:---:|:---|
| **Iniziale** | 0.0 | Round 0 |
| **Crescita** | Incrementi per scenario | Vedi matrice eventi |

**Distribuzione popolazione round 50:**
- μ ≈ 0.22
- σ ≈ 0.04
- Range: [0.15, 0.30]

---

## MATRICE EVENTI — Moltiplicatori Coefficienti

### V1 — MUTUA ESCLUSIONE VISIVA

| Coefficiente | Moltiplicatore | Valore assoluto (esempio) |
|:---|:---:|:---|
| aspiration | **×0.98** | 0.650 → 0.637 |
| selfMirror | **×1.02** | 0.350 → 0.357 |
| marketAwareness | **+0.003** | 0.100 → 0.103 |
| culturalPull | = | invariato |

---

### V2 — RICEVO ATTRAZIONE NON RICAMBIATA

| Coefficiente | Moltiplicatore | Valore assoluto (esempio) |
|:---|:---:|:---|
| aspiration | **×0.99** | 0.650 → 0.644 |
| selfMirror | **×1.01** | 0.350 → 0.354 |
| marketAwareness | **+0.002** | 0.100 → 0.102 |
| culturalPull | = | invariato |

**myDesired_X:**
- Rinforzo **+0.03** verso tratti di j (tutte le variabili)

---

### V3 — ESPRIMO ATTRAZIONE, VENGO RESPINTO

| Coefficiente | Moltiplicatore | Valore assoluto (esempio) |
|:---|:---:|:---|
| aspiration | **×0.97** | 0.650 → 0.631 |
| selfMirror | **×1.03** | 0.350 → 0.361 |
| marketAwareness | **+0.004** | 0.100 → 0.104 |
| culturalPull | = | invariato |

**myDesired_X:**
- Allontano **−0.02** da tratti di j (tutte le variabili)

---

### S1 — MUTUAL VISUAL, MUTUAL NO SEXUAL

| Coefficiente | Moltiplicatore | Valore assoluto (esempio) |
|:---|:---:|:---|
| aspiration | **×0.96** | 0.650 → 0.624 |
| selfMirror | **×1.04** | 0.350 → 0.364 |
| marketAwareness | **+0.005** | 0.100 → 0.105 |
| culturalPull | = | invariato |

---

### S2a — MUTUAL VISUAL, IO SÌ SEX, LORO NO

| Coefficiente | Moltiplicatore | Valore assoluto (esempio) |
|:---|:---:|:---|
| aspiration | **×0.97** | 0.650 → 0.631 |
| selfMirror | **×1.03** | 0.350 → 0.361 |
| marketAwareness | **+0.004** | 0.100 → 0.104 |
| culturalPull | = | invariato |

**myDesired_X:**
- Allontano **−0.03** da tratti SESSUALI di j (BottomTop, SubDom, KinkLevel, PenisSize)

---

### S2b — MUTUAL VISUAL, LORO SÌ SEX, IO NO

| Coefficiente | Moltiplicatore | Valore assoluto (esempio) |
|:---|:---:|:---|
| aspiration | **×0.98** | 0.650 → 0.637 |
| selfMirror | **×1.02** | 0.350 → 0.357 |
| marketAwareness | **+0.003** | 0.100 → 0.103 |
| culturalPull | = | invariato |

**myDesired_X:**
- Rinforzo **+0.02** verso tratti SESSUALI di j

---

### S3 — FULL MATCH (SUCCESSO)

| Coefficiente | Moltiplicatore | Valore assoluto (esempio) |
|:---|:---:|:---|
| aspiration | **×1.01** | 0.650 → 0.657 (ceiling 0.75) |
| selfMirror | **×0.99** | 0.350 → 0.347 |
| marketAwareness | **+0.010** | 0.100 → 0.110 |
| culturalPull | **condizionale** | vedi sotto |

**culturalPull adjustment:**
```python
alignment = partner_alignment_with_ideal(j)
if alignment > 0.70:
    culturalPull += 0.01 × alignment  # es: +0.008 se align=0.80
else:
    culturalPull *= 0.99
```

**myDesired_X:**
- Rinforzo **+0.12** verso tratti di j (TUTTE le variabili)

---

## LEARNING RATES

### myDesired_X Adjustment

| Scenario | LR | Direzione | Variabili |
|:---|:---:|:---:|:---|
| V2 (ricevo) | **+0.03** | Verso j | Tutte |
| V3 (respinto) | **−0.02** | Da j | Tutte |
| S2a (io sì, j no sex) | **−0.03** | Da j | Solo sessuali |
| S2b (j sì, io no sex) | **+0.02** | Verso j | Solo sessuali |
| S3 (full match) | **+0.12** | Verso j | Tutte |

---

### observedCulturalIdeal Update

| Parametro | Valore |
|:---|:---:|
| **LR_OBSERVED_IDEAL** | 0.10 |

**Formula:**
```python
new_ideal = mean([profiles[j] for j in top_30_percent_matches])
observedIdeal = old + 0.10 × (new_ideal - old)
```

---

### estimatedPopMean Update

**Round 0 (bootstrap):**
```python
estimatedPopMean[var] = mean(first_batch_seen)
```

**Round 1+ (media ponderata esatta):**
```python
new_mean = (old_mean × n_prev + batch_mean × n_curr) / (n_prev + n_curr)
```

---

## SOGLIE ADATTIVE

### Inizializzazione

| Parametro | Valore |
|:---|:---:|
| **INITIAL_VIS_THRESHOLD** | 0.42 |
| **INITIAL_SEX_THRESHOLD** | 0.42 |
| **THRESHOLD_ABSOLUTE_FLOOR** | 0.02 |
| **THRESHOLD_CEILING** | 0.95 |
| **FLOOR_COEFFICIENT** | 0.75 |

---

### Aggiustamenti per Scenario

| Scenario | my_visThreshold | my_sexThreshold |
|:---|:---:|:---:|
| V1 (mutual exclusion) | **−2.0%** decay | = |
| V2 (ricevo) | = | = |
| V3 (respinto) | = | = |
| S1 (vis yes, sex no) | **+0.008** | **−1.5%** decay |
| S2a (io sì, j no sex) | **+0.008** | **×0.988** |
| S2b (j sì, io no sex) | **+0.008** | = |
| S3 (full match) | **+0.025** | **+0.025** |

**Threshold decay formula (V1, S1):**
```python
delta = rate × current_threshold
new_threshold = max(floor, current_threshold - delta)
```

**Esempi:**
- V1: `0.42 × 0.020 = 0.0084` → `0.42 - 0.0084 = 0.411`
- S1 sex: `0.42 × 0.015 = 0.0063` → `0.42 - 0.0063 = 0.414`

---

### Floor Update (Adaptive)

**Calcolo floor:**
```python
recv_vis_mean = my_receivedAttractionSum / max(1, my_receivedAttractionN)
my_visFloor = max(0.02, recv_vis_mean × 0.75)

recv_sex_mean = my_receivedSexAttractionSum / max(1, my_receivedSexAttractionN)
my_sexFloor = max(0.02, recv_sex_mean × 0.75)
```

**Distribuzione popolazione round 50:**
- **my_visFloor:** μ ≈ 0.12, range [0.02, 0.35]
- **my_sexFloor:** μ ≈ 0.10, range [0.02, 0.30]

---

## FRUSTRAZIONE

### Incrementi per Scenario

| Scenario | my_visFrustration | my_sexFrustration | my_isolationFrustration |
|:---|:---:|:---:|:---|
| V1 | **+0.12** | = | = |
| V2 | = | = | **×0.95** (riduzione) |
| V3 | **+0.10** | = | = |
| S1 | = | **+0.18** | = |
| S2a | = | **+0.15** | = |
| S2b | = | **+0.08** | = |
| S3 | **×0.80** | **×0.80** | **×0.70** |

---

### Parametri Decay

| Parametro | Valore |
|:---|:---:|
| **FRUST_VIS_V1** | 0.12 |
| **FRUST_VIS_V3** | 0.10 |
| **FRUST_SEX_S1** | 0.18 |
| **FRUST_SEX_S2A** | 0.15 |
| **FRUST_SEX_S2B** | 0.08 |
| **FRUST_ISO_MULT_V2** | 0.95 |
| **FRUST_DECAY_S3** | 0.80 |
| **FRUST_ISO_DECAY_S3** | 0.70 |

---

## SELFESTEEM SOCIOMETRO

### Segnali Outcome

| Outcome | Segnale |
|:---|:---:|
| **Full match** | 0.68 |
| **Visual match (no sex)** | 0.51 |
| **No match** | 0.35 |

---

### Learning Rates

| Parametro | Valore |
|:---|:---:|
| **SE_OUTCOME_LR** | 0.08 |
| **SE_ANCHOR_LR** | 0.015 |
| **SE_FRUSTRATION_EROSION** | 0.30 |

**Formula:**
```python
# Outcome signal
if n_full > 0:
    signal = 0.68
elif n_vis_match > 0:
    signal = 0.51
else:
    signal = 0.35

# Frustrazione erode anchor
frust_norm = min(1.0, my_visFrustration / 3.0)
anchor_eff = max(0.002, 0.015 × (1.0 - 0.30 × frust_norm))

# Update
MySelfEsteem_dynamic += 0.08 × (signal - MySelfEsteem_dynamic)
MySelfEsteem_dynamic += anchor_eff × (MySelfEsteem_static - MySelfEsteem_dynamic)
```

---

## RISULTATI SIMULAZIONE — Distribuzione Finali (Round 50)

### Coefficienti

| Coefficiente | μ | σ | Range |
|:---|:---:|:---:|:---|
| **aspiration** | 0.548 | 0.124 | [0.32, 0.75] |
| **selfMirror** | 0.412 | 0.098 | [0.24, 0.62] |
| **culturalPull** | 0.182 | 0.028 | [0.15, 0.26] |
| **marketAwareness** | 0.218 | 0.042 | [0.15, 0.30] |

---

### Soglie

| Soglia | μ | σ | Range |
|:---|:---:|:---:|:---|
| **my_visThreshold** | 0.385 | 0.112 | [0.08, 0.68] |
| **my_sexThreshold** | 0.362 | 0.128 | [0.05, 0.72] |
| **my_visFloor** | 0.118 | 0.062 | [0.02, 0.35] |
| **my_sexFloor** | 0.095 | 0.058 | [0.02, 0.30] |

---

### Frustrazione

| Variabile | μ | σ | Range |
|:---|:---:|:---:|:---|
| **my_visFrustration** | 1.28 | 0.85 | [0.02, 4.20] |
| **my_sexFrustration** | 1.45 | 0.92 | [0.05, 4.80] |
| **my_isolationFrustration** | 0.68 | 0.52 | [0.00, 2.50] |

---

### Match Count

| Metrica | Totale | Pop % |
|:---|:---:|:---:|
| **my_fullMatchCount totali** | ~2,800 | — |
| **Pop con ≥1 match** | ~450 / 700 | 64% |
| **my_visMatchCount totali** | ~4,200 | — |

---

### MySelfEsteem

| Metrica | Valore |
|:---|:---:|
| **MySelfEsteem_static μ** | 0.481 |
| **MySelfEsteem_dynamic μ** | 0.492 |
| **Δ (dyn - stat)** | +0.011 |
| **r(static, dynamic)** | +0.72 |

---

## CORRELAZIONI myDesired_X — Learned vs v2.3 Static

| Variabile | r (learned, v2.3) | μ learned | μ v2.3 | Interpretazione |
|:---|:---:|:---:|:---:|:---|
| **Muscle** | +0.45 | 0.512 | 0.485 | Moderata convergenza |
| **Beauty** | +0.68 | 0.622 | 0.638 | Alta (universale) |
| **Slim** | +0.52 | 0.488 | 0.502 | Moderata |
| **Hair** | +0.38 | 0.465 | 0.448 | Bassa (culturale) |
| **Masculinity** | +0.42 | 0.558 | 0.542 | Moderata |
| **Height** | +0.58 | 0.595 | 0.612 | Moderata-alta |
| **Age** | +0.38 | 0.385 | 0.368 | Bassa (appreso) |
| **BottomTop** | +0.82 | 0.502 | 0.498 | Alta (complementarità innata) |
| **SubDom** | +0.48 | 0.468 | 0.455 | Moderata |
| **KinkLevel** | +0.44 | 0.425 | 0.412 | Moderata |
| **PenisSize** | +0.51 | 0.545 | 0.528 | Moderata |

**Pattern:**
- **Alta convergenza** → tratti universali/innati (Beauty, BottomTop)
- **Bassa convergenza** → tratti culturali/appresi (Hair, Age)
- **Media learned > media v2.3** → deriva verso baseline "attraente" (0.65)

---

## CORRELAZIONI COEFFICIENTI — Emergenti

| Coppia | r | Interpretazione |
|:---|:---:|:---|
| **aspiration × selfMirror** | **−0.52** | Anticorrelati (non reciproci perfetti) |
| **aspiration × my_fullMatchCount** | **+0.68** | Successo → aspiration alta |
| **selfMirror × my_fullMatchCount** | **−0.58** | Successo → selfMirror bassa |
| **culturalPull × my_fullMatchCount** | **+0.32** | Lieve correlazione positiva |
| **marketAwareness × my_encounterCount** | **+0.85** | Cresce con esperienza |
| **my_visThreshold × my_receivedAttractionMean** | **+0.72** | Calibrazione sul ricevuto |

---

## CONFRONTO VERGINE v1.0 vs VERGINE TOTALE v1.1

| Aspetto | Vergine v1.0 | Vergine Totale v1.1 |
|:---|:---|:---|
| **myDesired_X** | Fissi (v2.3 formula) | **Adattivi (apprendimento)** |
| **Coefficienti** | Impliciti | **Espliciti (4 coefficienti)** |
| **Ideale culturale** | Da letteratura (CULTURAL_IDEAL) | **Osservato (top 30% match)** |
| **Stima popolazione** | Da POP_MEAN | **Campionata (bootstrap)** |
| **Soglie** | Adattive | Adattive (stesso meccanismo) |
| **Matrice eventi** | 3 scenari (no match, vis, full) | **7 scenari (V1-V3, S1-S3)** |
| **Match rate** | ~55% | ~64% |
| **Convergenza** | Rapida (round 15) | Lenta (round 30-40) |

---

## NOTE CALIBRAZIONE

### Valori Empirici Attesi

**Coefficienti round 50 (per tier):**

| Tier | aspiration | selfMirror | Somma |
|:---|:---:|:---:|:---:|
| Bottom (0 match) | 0.32 | 0.58 | 0.90 |
| Mid (1-3 match) | 0.55 | 0.38 | 0.93 |
| Top (5+ match) | 0.72 | 0.28 | 1.00 |

**Somma (asp+sm):** Non vincolata a 1.0 → varianza [0.85, 1.05]

---

### Sensibilità Parametri

**ASPIRATION_MEAN ±0.05:**
- 0.60 → match rate −8%, aspiration finale μ=0.51
- 0.70 → match rate +5%, aspiration finale μ=0.58

**LR_DESIRED_S3_FULL_MATCH ±0.03:**
- 0.09 → convergenza lenta (+10 round), r(learned,v2.3) −0.08
- 0.15 → convergenza rapida (−5 round), r(learned,v2.3) +0.12

**SUBSAMPLE_SIZE:**
- 20 → varianza alta, convergenza instabile
- 30 → sweet spot (attuale)
- 50 → convergenza rapida ma varianza bassa

---

## FILE CORRELATI

- `macchina_vergine_totale_1_1.py` — Codice implementazione
- `algorithm_walkthrough_vergine_1_1.md` — Spiegazione discorsiva
- `foundation_vergine_totale.md` — Architettura generale
- `open_questions_vergine_totale.md` — TODO e decisioni pendenti
- `bibliography_vergine.md` — Fonti letteratura
