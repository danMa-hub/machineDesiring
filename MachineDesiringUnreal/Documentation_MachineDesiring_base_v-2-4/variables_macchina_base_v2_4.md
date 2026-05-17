# MACCHINA DEL DESIDERIO — Variables & Conventions

**versione 2.4 — aggiornare quando si stabilizzano parametri**

---

## Convenzioni di nomenclatura — REGOLE OBBLIGATORIE

### Prefissi per scope

| Prefisso | Scope | Esempio |
|:---|:---|:---|
| `My` (PascalCase) | Tratto di profilo NPC — calcolato localmente | `MyMuscle`, `MyAge` |
| `my` (camelCase) | Derivata scalare/categorica dal profilo | `myAgeNorm`, `myTribeTag` |
| `myDesired_` | Target di preferenza assortativa | `myDesired_Muscle` |
| `myWeights` | Pesi individuali normalizzati | `myWeightsVisual` |
| `my` (bias) | Bias osservatore — stabile per persona | `myEthBias`, `myPozBias` |
| `myPop_` | Metrica per-NPC calcolata sull'intera popolazione | `myPop_visAttraction` |
| `myOut_` | Score/contatore uscente per-NPC (A→B) | `myOut_visAttraction_cruise`, `myOut_visAttractionCount` |
| `myIn_` | Score/contatore entrante per-NPC (B→A ricevuto) | `myIn_visAttractionCount` |
| `my_` (underscore) | Contatore o stato runtime per-NPC | `my_visMatchCount_cruise` |
| `pair_` | Risultato bool per incontro coppia | `pair_visMatched` |
| `pop_` | Metrica aggregata su tutti gli NPC (singolo valore) | `pop_neverVisMatched_cruise` |
| `pairs_` | Metrica aggregata su tutte le coppie possibili | `pairs_visCompatibilityRate` |
| `myAcc_` | Valore cumulativo round-based (Vergine) | `myAcc_visMatchCount_cruise` |
| `myCurr_` | Snapshot del round corrente (Vergine) | `myCurr_visAttraction_cruise` |

### Suffissi per modalità

| Suffisso | Quando usarlo |
|:---|:---|
| `_cruise` | Metrica valida solo in modalità cruising (two-step) |
| `_app` | Metrica valida solo in modalità app (single-pass) |
| nessuno | Metrica indipendente dalla modalità |

### Regole aggiuntive

- **Verbi al participio passato** per bool di evento: `visMatched`, non `visMatch`
- **camelCase** per tutto ciò che segue il prefisso
- **Singolare** per scalari, **plurale** per liste: `myTribeTag`, `myKinkTypes`
- **Dimensione esplicita**: ogni nome deve contenere `vis`, `sex`, o `global` dove rilevante

---

## IndividualProfile — variabili principali

| Variabile | Tipo | Media | SD | Distribuzione | Note |
|:---|:---|:---|:---|:---|:---|
| **CORPO** | | | | | |
| MyMuscle | dep | 0.30 | 0.15 | gauss - age piecewise | picco 35yr, calo accelerato >60 |
| MySlim | dep | 0.48 | 0.22 | gauss - age×0.15 + muscle bonus | NHANES calibrato |
| MyHair | dep | 0.46 | 0.18 | gauss + age + ethnicity | |
| MyBodyMod | dep | 0.18 | exp | exp + queerness - age | fuori dal match |
| **PRESENZA** | | | | | |
| MyBeauty | dep | 0.40 | 0.15 | gauss - age×0.20 | decay forte (Schope 2005) |
| MyMasculinity | dep | 0.62 | 0.18 | gauss+hair-queer+height | |
| MyPolish | ind | 0.44 | 0.18 | gauss | fuori dal match |
| MyQueerness | ind | bimod | — | bimodale 0.20/0.72 | fuori dal match |
| MyBodyDisplay | dep | 0.22 | exp | exp + muscle×0.10 | UE5 vestiti, fuori dal match |
| **IDENTITÀ** | | | | | |
| MyAge | ind | 38.0 | 15.7 | gauss clamp 18-100 | |
| myAgeNorm | der | — | — | (MyAge-18)/82 | usato nel match |
| MyHeight | ind | 177.0 | 7.0 | gauss (cm) | |
| myHeightNorm | der | — | — | (MyHeight-160)/35 | usato nel match |
| MyEthnicity | ind | — | — | White0.58/Latino0.16/Black0.12/Asian0.08/ME0.02/Mixed0.04 | |
| **SESSUALE** | | | | | |
| MyBottomTop | dep | trimod | — | B28%/V50%/T22% | complementare |
| MySubDom | dep | 0.44 | 0.20 | gauss + BT×0.10 | |
| MyKinkLevel | ind | 0.18 | exp | exp inversa | |
| MySexActDepth | dep | 0.38 | 0.20 | gauss + SexExtremity | fuori dal match |
| MySexExtremity | dep | 0.32 | 0.18 | gauss + KinkLevel | condiziona myKinkTypes |
| MyPenisSize | ind | 0.50 | 0.15 | gauss | indipendente (Veale 2015) |
| MyPozStatus | dep | — | — | age-dependent, CDC 2022 | categorica |
| MySafetyPractice | dep | — | — | condizionata PozStatus | categorica |
| **PSICOLOGICHE** | | | | | |
| MyOpenClosed | ind | 0.55 | 0.22 | gauss | futuro uso relazionale |
| MyRelDepth | ind | 0.50 | 0.22 | gauss | idem |
| **DERIVATE** | | | | | |
| MySelfEsteem | der | ~0.48 | ~0.16 | Beauty×0.20+Muscle×0.15+... | base 0.21, per BehaviorTree |
| myTribeTag | der | — | — | prototype scoring euclideo | SOLO LOD/debug |
| myKinkTypes | der | — | — | flag multipli | per profilo e narrativa |

---

## MatingPreferences

Formula: `myDesired_X = r_self×my + r_ideal×ideal + r_pop×pop_mean + noise`

### ASSORT_COEFFS (v2.4)

| Variabile | r_self | r_ideal | r_pop | noise_sd | Logica | Fonte |
|:---|:---|:---|:---|:---|:---|:---|
| Muscle | 0.25 | 0.35 | 0.40 | 0.12 | aspirazionale | Luo 2017, Bruch 2018 |
| Slim | 0.25 | 0.35 | 0.40 | 0.12 | aspirazionale | Luo 2017 |
| Beauty | 0.20 | 0.40 | 0.40 | 0.10 | aspirazionale | Luo 2017 |
| Masculinity | 0.45 | 0.20 | 0.35 | 0.12 | assort. forte | Bailey 1997 |
| Height | 0.30 | 0.20 | 0.50 | 0.10 | aspirazionale | Valentova 2014 |
| Hair | 0.15 | 0.00 | 0.85 | 0.28 | bipolare | Muscarella 2002 |
| Age | 0.70 | 0.20 | 0.10 | 0.06 | assort.+youth | Antfolk 2017 |
| BottomTop | −0.90 | 0.00 | 0.10 | 0.12 | complementare | Moskowitz 2008 |
| SubDom | −0.85 | 0.00 | 0.15 | 0.12 | complementare | Bailey 1997 |
| KinkLevel | 0.75 | 0.00 | 0.25 | 0.10 | speculare | BDSM lit |

**Nota nomenclatura:** `r_self` = ancoraggio al proprio tratto (ex `r_assort`). `r_ideal` = pull verso ideale culturale (ex `r_aspir`). `r_pop` invariato.

### Variabili fuori da ASSORT_COEFFS

| Variabile | Formula | Fonte |
|:---|:---|:---|
| `myDesired_PenisSize` | 0.50 + (1−BT)×0.25 + noise(0,0.12) | GMFA 2017 |

### Ideale culturale e media popolazione

| Variabile | CULTURAL_IDEAL | POP_MEAN |
|:---|:---|:---|
| Muscle | 0.75 | 0.30 |
| Slim | 0.70 | 0.50 |
| Beauty | 0.80 | 0.45 |
| Masculinity | 0.75 | 0.58 |
| Height | 0.65 | 0.50 |
| Age | 0.122 (=28yr) | 0.249 (=38.4yr) |
| Hair | — | 0.46 |

---

## Pesi base — letteratura gay-specifica

### myWeightsVisual (7 variabili — cruising fase 1)

| Variabile | Peso base | Noise SD | Fonte |
|:---|:---|:---|:---|
| Muscle | 0.28 | 30% | Swami & Tovée 2008 |
| Beauty | 0.23 | 30% | Levesque & Vichesky 2006 |
| Slim | 0.15 | 30% | Swami & Tovée 2008 |
| Masculinity | 0.15 | 40% | Bailey 1997 (polarizzato) |
| Age | 0.07 | 30% | Schope 2005 |
| Hair | 0.05 | 40% | Martins 2008 (polarizzato) |
| Height | 0.04 | 30% | Valentova 2014 |

### myWeightsSexual (4 variabili + modulazione — cruising fase 2)

| Variabile | Peso base | Modulazione | Fonte |
|:---|:---|:---|:---|
| BottomTop | 0.40 | ×(1 + bt_extremity × 0.60) | Moskowitz 2008 |
| SubDom | 0.28 | — | Bailey 1997 |
| KinkLevel | 0.17 | ×(1 + KL × 0.80) | Parsons 2010 |
| PenisSize | 0.15 | ×(1 + bottomness × 0.80) | GMFA 2017 |

### myWeightsSingle (11 variabili — app, pesi normalizzati)

Normalizzazione unica: pesi raw / somma(pesi raw = 1.97). Nessuno split vis/sex artificiale.

| Variabile | Peso norm. | Raw | Note |
|:---|:---|:---|:---|
| BottomTop | **0.203** | 0.40 | domina — vincolante funzionale |
| Muscle | 0.142 | 0.28 | |
| SubDom | 0.142 | 0.28 | |
| Beauty | 0.117 | 0.23 | |
| KinkLevel | 0.086 | 0.17 | |
| Slim | 0.076 | 0.15 | |
| Masculinity | 0.076 | 0.15 | |
| PenisSize | 0.076 | 0.15 | |
| Age | 0.036 | 0.07 | |
| Hair | 0.025 | 0.05 | |
| Height | 0.020 | 0.04 | |

Stesse modulazioni BT extremity / KinkLevel / bottomness applicate. Dealbreaker BT: bt_distance > 0.60 → ×0.15. Versatile (desired≈0.46): distanza max 0.44 → mai dealbreaker.

---

## Bias sistemici — moltiplicatori post-distanza

### Etnia (OkCupid 2009 gay-specific)

| Etnia | Moltiplicatore |
|:---|:---|
| MiddleEastern | 1.07 |
| White | 1.00 |
| Latino | 0.95 |
| Mixed | 0.95 |
| Asian | 0.85 |
| Black | 0.80 |

Observer noise: `myEthBias` SD=0.40. Floor: 0.50.

### Sierostatus (CDC 2022, GLAAD 2022)

| Status | Moltiplicatore |
|:---|:---|
| None | 1.00 |
| PozUndetectable | 0.70 |
| Poz | 0.36 |

Observer noise: `myPozBias` SD=0.50. Floor: 0.20.

---

## Metriche di output — nomenclatura completa

### Score direzionali per incontro (non memorizzati)

| Nome | Modalità | Cosa misura |
|:---|:---|:---|
| `myOut_visAttraction_cruise[i][j]` | cruising | quanto i trova j visivamente attraente |
| `myOut_sexAttraction_cruise[i][j]` | cruising | quanto i trova j sessualmente compatibile |
| `myOut_attraction_app[i][j]` | app | score combinato i→j |

### Bool per incontro (non memorizzati)

| Nome | Modalità | Cosa misura |
|:---|:---|:---|
| `pair_visMatched` | cruising | mutual visual realizzato |
| `pair_visSexMatched` | cruising | full match vis+sex realizzato |
| `pair_fullMatched` | app | match app realizzato |

### Metriche per-NPC — appeal (calcolate sulla popolazione)

| Nome | Modalità | Cosa misura |
|:---|:---|:---|
| `myPop_visAttraction` | cruising | media attrazioni visive ricevute da tutti |
| `myPop_sexAttraction` | cruising | media attrazioni sessuali ricevute (gated su mutual visual) |
| `myPop_globalAttraction` | entrambe | 0.60×vis + 0.40×sex; se 0 mutual → uguale a vis |

### Metriche per-NPC — contatori e indici

| Nome | Modalità | Cosa misura |
|:---|:---|:---|
| `myOut_visAttractionCount` | cruising | quante j ha trovato visivamente attraenti (soglia) |
| `myIn_visAttractionCount` | cruising | quanti lo trovano visivamente attraente (soglia) |
| `my_visAttractionBalance` | cruising | myIn / myOut — >1 desiderato più di quanto desidera |
| `my_visMatchCount_cruise` | cruising | mutual visual accumulati |
| `my_visSexMatchCount_cruise` | cruising | full match accumulati |
| `my_visToSexFrustrationRate` | cruising | (visMatch − sexMatch) / visMatch |
| `myOut_visAttractionCount` | app | quante j ha trovato attraenti |
| `myIn_visAttractionCount` | app | quanti lo trovano attraente |
| `my_visAttractionBalance` | app | myIn / myOut |
| `my_matchCount_app` | app | match app accumulati |

### Metriche popolazione — persone

| Nome | Modalità | Cosa misura |
|:---|:---|:---|
| `pop_neverVisDesired_cruise` | cruising | % NPC mai trovati attraenti da nessuno |
| `pop_neverVisMatched_cruise` | cruising | % NPC senza mai mutual visual |
| `pop_neverSexMatched_cruise` | cruising | % NPC senza mai full match |
| `pop_neverMatched_app` | app | % NPC senza mai match app |
| `pop_visAttractionPolarization` | cruising | Gini su myPop_visAttraction |

### Metriche popolazione — coppie

| Nome | Modalità | Cosa misura |
|:---|:---|:---|
| `pairs_visCompatibilityRate` | cruising | % coppie con mutual visual |
| `pairs_visSexCompatibilityRate` | cruising | % coppie con full match |
| `pairs_fullCompatibilityRate_app` | app | % coppie con match app |
| `pairs_bottomTopIncompatibilityShare` | cruising | quota fallimenti per BT incompatibile |
| `pairs_kinkIncompatibilityShare` | cruising | quota fallimenti per kink mismatch |

### Vergine — metriche aggiuntive (round-based)

| Nome | Tipo | Cosa misura |
|:---|:---|:---|
| `myAcc_visAttraction_cruise` | float | media attrazioni ricevute accumulata fino al round R |
| `myCurr_visAttraction_cruise` | float | snapshot round corrente |
| `my_visThreshold` | float | soglia visiva adattiva |
| `my_sexThreshold` | float | soglia sessuale adattiva |
| `my_visFrustration` | float | accumulo frustrazione visiva |
| `my_sexFrustration` | float | accumulo frustrazione sessuale |

---

## Risultati simulazione v2.4 — riferimento (N=700, seed=42)

### Cruising (two-step)

```
myOut_visAttraction mean             : 0.212
myOut_visAttractionCount mean/med/max: 132.1 / 119 / 431
myIn_visAttractionCount  mean/med/max: 132.1 / 78 / 597
my_visAttractionBalance  mean/med    : 2.306 / 0.757   (mediana < media → distribuzione asimmetrica)
pop_neverVisDesired_cruise           : 25 (4%)
pop_neverVisMatched_cruise           : 97 (14%)
pop_neverSexMatched_cruise           : 357 (51%)
pairs_visCompatibilityRate           : 7876 (3.22%)
pairs_visSexCompatibilityRate        : 785 (0.32%)
pairs_vis→sex conversion             : 10.0%
my_visToSexFrustrationRate mean      : 0.907  (su 603 NPC con ≥1 visMatch)
pop_visAttractionPolarization        : 0.313
myPop_visAttraction mean/sd          : 0.202 / 0.113
myPop_sexAttraction mean/sd          : 0.178 / 0.123  (su 603 NPC con mutual visual)
myPop_globalAttraction mean/sd       : 0.187 / 0.088
r(visAttr, globalAttr)               : +0.845
r(visAttr, visMatchCount)            : +0.721
r(visAttr, sexMatchCount)            : +0.531
r(globalAttr, sexMatchCount)         : +0.579
r(visAttr_A, visAttr_B) nei match    : +0.236
pairs_bottomTopIncompatibilityShare  : 24%
pairs_kinkIncompatibilityShare       : 8%
General distance                     : 69% (fonte dominante di frustrazione)
White neverVisMatched 12% / Latino 18% / Black 14% / Asian 19%
None  neverSexMatched 11% / PozUndet 25% / Poz 29%
```

### App (single-pass) — nuovi pesi normalizzati v2.4

```
myOut_visAttractionCount mean/med/max: 113.3 / 103 / 381
myIn_visAttractionCount  mean/med/max: 113.3 / 92 / 431
my_visAttractionBalance  mean/med    : 1.943 / 0.979   (più simmetrico vs cruising)
pop_neverMatched_app                 : 19 (3%)          (era 5% v2.3 — migliorato)
pairs_fullCompatibilityRate_app      : 12934 (5.29%)    (era 4.34% v2.3 — aumentato)
pop_visAttractionPolarization        : 0.222
myPop_visAttraction mean/sd          : 0.186 / 0.073
r(visAttr, visMatchCount)            : +0.749
r(MyMuscle, myPop_visAttraction)     : +0.275
r(MyBeauty, myPop_visAttraction)     : +0.265
r(MyPenisSize, myPop_visAttraction)  : +0.268  (sale vs cruising +0.029 — BT media influenza)
r(visAttr_A, visAttr_B) nei match    : +0.244
White neverMatched 1% / Latino 7% / Black 5% / Asian 9%
None  neverMatched 2% / PozUndet 4% / Poz 17%
```

**Differenze chiave v2.4 vs v2.3 app:** con pesi normalizzati (BottomTop 0.203 vs 0.152 precedente), l'app diventa leggermente più inclusiva in termini di match rate (5.29% vs 4.34%) e meno escludente (3% vs 5% neverMatched). Il balance medio più vicino a 1 in app vs cruising indica maggiore simmetria di mercato nel contesto single-pass. `r(PenisSize, visAttr)` sale da +0.029 a +0.268 in app perché PenisSize è ora visibile direttamente (peso 0.076 vs 0 in cruising visivo).

### Distribuzione profili (validazione)

```
MyAge            mean=38.335 sd=14.150  (target=38.00 ✓)
MyMuscle         mean=0.287  sd=0.146   (target=0.30  ✓)
MySlim           mean=0.465  sd=0.220   (target=0.48  ✓)
MyBeauty         mean=0.413  sd=0.154   (target=0.40  ✓)
MyMasculinity    mean=0.614  sd=0.190   (target=0.62  ✓)
MyHair           mean=0.505  sd=0.195   (target=0.46  ⚠ +0.045)
MyKinkLevel      mean=0.184  sd=0.177   (target=0.18  ✓)
MyPenisSize      mean=0.497  sd=0.147   (target=0.50  ✓)
MySelfEsteem     mean=0.457  sd=0.160   (target=0.48  ✓)
MySubDom         mean=0.442  sd=0.200   (target=0.44  ✓)
```

MyHair lievemente sopra target (+0.045) — Hair ha alta varianza e piccolo drift da ethnicity modifier. Non critico.

