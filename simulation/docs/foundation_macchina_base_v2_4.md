# MACCHINA DEL DESIDERIO — Foundation

**versione 2.4 — documento stabile (cambia solo con versioni maggiori)**

---

## Concept

Installazione artistica interattiva sviluppata in Unreal Engine 5.7. Esplora le dinamiche relazionali e sessuali della comunità gay maschile contemporanea attraverso una simulazione autonoma di personaggi che si muovono, si cercano, si desiderano e si rifiutano in uno spazio virtuale.

Omaggio alla Macchina Celibe di Marcel Duchamp — un sistema che produce desiderio senza mai risolverlo. L'incompatibilità strutturale tra desiderio e realtà è il motore narrativo dell'opera.

**Temi:** desiderio come motore irrisolvibile, stigma e gerarchia estetica nella cultura gay, solitudine strutturale, dinamiche di cruising e chemsex, il corpo come capitale sociale.

---

## Architettura generale v2.4

### Modello di attrazione: Distanza Euclidea Pesata

Fonte: Conroy-Beam (2016-2022) — N=14487, 45 paesi. **NOTA: campione eterosessuale.** Il modello matematico è valido universalmente; i pesi vengono dalla letteratura gay-specifica.

```
d = sqrt( Σ w_i × (myDesired_i - trait_i)² )
attraction = exp( -d² / (2σ²) )
```

### Dual mode — Cruising e App

**MODE_CRUISING (two-step)** — incontro fisico (cruising, bar, darkroom).
- Fase 1: visual — 7 variabili + bias post-distanza. σ=0.13.
- Fase 2: sexual — 4 variabili. σ=0.13. Solo se `pair_visMatched = true`.
- Produce frustrazione sessuale strutturale (~88% delle `pair_visMatched` non arriva a `pair_visSexMatched`).

**MODE_APP (single-pass)** — dating app (Grindr, Scruff).
- Calcolo unico: 11 variabili insieme + bias post-distanza. σ=0.15.
- Pesi: raw dalla letteratura, normalizzati a somma 1 senza split vis/sex artificiale. BottomTop domina (0.203) perché funzionalmente vincolante.
- Dealbreaker BT: bt_distance > 0.60 → ×0.15. I versatili non raggiungono mai il dealbreaker (distanza max 0.44).
- Nessuna frustrazione sessuale per definizione.

### Strutture dati principali

- `IndividualProfile` — chi è il personaggio (prefisso `My` PascalCase)
- `MatingPreferences` — cosa cerca (prefisso `myDesired_`)
- `myWeightsVisual` / `myWeightsSexual` / `myWeightsSingle` — pesi individuali normalizzati
- `myEthBias` / `myPozBias` — sensibilità individuale ai bias (stabile per persona)
- `myKinkTypes` — lista di flag kink multipli per personaggio

### Appeal emergente

Fonte: Conroy-Beam (2018). Tripartito in cruising:
- `myPop_visAttraction` = media attrazioni visive ricevute da tutti
- `myPop_sexAttraction` = media attrazioni sessuali ricevute, gated su `pair_visMatched`
- `myPop_globalAttraction` = 0.60×vis + 0.40×sex; se 0 mutual visual → uguale a visAttraction
- In modalità APP: `myPop_globalAttraction` unico (vis e sex collassano)

### Metriche individuali nuove (v2.4)

- `my_visAttractionBalance` = `myIn_visAttractionCount / myOut_visAttractionCount` — >1 desiderato più di quanto desidera; misura asimmetria di mercato per-NPC
- `my_visToSexFrustrationRate` = `(my_visMatchCount_cruise − my_visSexMatchCount_cruise) / my_visMatchCount_cruise` — quota di vis match che non converte

### Macchina Vergine — simulazione dinamica

Estensione con soglie adattive per-NPC: `my_visThreshold` / `my_sexThreshold`, `my_visFrustration` / `my_sexFrustration`. 700 NPC, gruppi da 50 per round, convergenza ~10 round.

---

## Eliminazioni rispetto a v1.0

| Componente | Motivazione |
|:---|:---|
| Archetipi (6 tipi) | Nessuna validità scientifica come categoria di preferenza |
| FetishBoost | Costruzione interna senza supporto letteratura |
| AcceptanceRange | Sostituito da σ della gaussiana |
| ConformityScore | Sostituito da Appeal emergente dalla simulazione |
| generate_report() / matplotlib | Eliminato in v2.4 — output solo testo |

---

## Scelte metodologiche fondamentali — NON MODIFICARE

**1. Distribuzioni demografiche reali — non delle app**
MyAge media 38 anni (non 25), MyMuscle media 0.30 (non 0.60). Scelta artistica deliberata.

**2. Bias sistemici come realtà documentata**
`myEthBias` e `myPozBias` basati su dati empirici. NON attenuare per correttezza politica.

**3. Match in due fasi temporali distinte (cruising)**
NON calcolare visual e sexual simultaneamente nel cruising.

**4. myTribeTag solo per LOD e debug**
NON usare per match individuale diretto.

**5. myDesired_ da formula assortativa**
`myDesired_X = r_self×my + r_ideal×ideal + r_pop×pop_mean + noise`. Eccezioni: PenisSize (BT-condizionale), BottomTop e SubDom (complementari, r_self negativo).

**6. Normalizzazione pesi separata per step**
`myWeightsVisual`, `myWeightsSexual`, `myWeightsSingle` normalizzati separatamente.

**7. myEthBias / myPozBias generati una volta per persona**
NON rigenerare per ogni coppia.

**8. Pesi app: normalizzazione unica senza split artificiale**
Pesi raw dalla letteratura normalizzati a somma 1. Nessun rapporto vis/sex imposto — emerge dai dati. Nessuna fonte empirica quantificava un 62/38 su app gay.

**9. Pesi da letteratura gay-specifica**
Swami & Tovée 2008, Levesque & Vichesky 2006, Martins 2008, Bailey 1997, Moskowitz 2008, GMFA 2017.

**10. Nomenclatura fissa — vedi regole in variables_v2_4.md**
Ogni nuova variabile deve seguire le convenzioni di prefisso documentate.

---

## Pipeline generazione — ordine obbligatorio

### IndividualProfile

Passata 1 → variabili indipendenti (MyAge, MyHeight, MyEthnicity, hair_base, beauty_base, bodymod_base, bodydisplay_base, MyPolish, MyQueerness, MyKinkLevel, MyOpenClosed, MyRelDepth, MyPenisSize)

Passata 2 → variabili condizionate (myAgeNorm, myHeightNorm, MyHair, MyBodyMod, MyBottomTop, MyMuscle, MySlim, MyBeauty, MyMasculinity, MyBodyDisplay, MySexExtremity, MySexActDepth, MySubDom, MyPozStatus, MySafetyPractice)

Passata 3 → variabili derivate (MySelfEsteem, myTribeTag, myKinkTypes)

### MatingPreferences

Step 1 → `myDesired_X` per tutte le variabili in ASSORT_COEFFS
```
complementare (r_self < 0): myDesired = |r_self|×(1-my) + (1-|r_self|)×pop_mean
aspirazionale (r_ideal > 0): myDesired = r_self×my + r_ideal×ideal + r_pop×pop_mean
assortativo puro:            myDesired = r_self×my + (1-r_self)×pop_mean
+ noise gaussiano individuale
```

Step 2 → `myDesired_PenisSize` (BT-condizionale, fuori da ASSORT_COEFFS)

Step 3 → `myWeightsVisual`, `myWeightsSexual`, `myWeightsSingle` (BASE_WEIGHTS + noise 30-40%, normalizzati). Modulazione: BT extremity → peso BT; MyKinkLevel → peso KL; bottomness → peso PS

Step 4 → `myEthBias`, `myPozBias` (una volta per persona)

### Match — Cruising

```
Fase 1: myOut_visAttraction_cruise(A→B) >= threshold
     AND myIn_visAttraction_cruise(B→A) >= threshold   [= myOut(B→A)]
     → pair_visMatched = true

Fase 2 (solo se pair_visMatched):
     myOut_sexAttraction_cruise(A→B) >= threshold
     AND myOut_sexAttraction_cruise(B→A) >= threshold
     → pair_visSexMatched = true
```

In v2.4: soglia fissa 0.35. In Vergine: soglie adattive per-NPC.

### Match — App

```
myOut_attraction_app(A→B) >= threshold
AND myOut_attraction_app(B→A) >= threshold
→ pair_fullMatched = true
```

---

## Variabili nel match vs nel profilo

### Nel match VISUAL cruising (7 + 2 moltiplicatori)

| Variabile | Peso base | Noise | Tipo | Fonte |
|:---|:---|:---|:---|:---|
| MyMuscle | 0.28 | 30% | aspirazionale | Swami & Tovée 2008 |
| MyBeauty | 0.23 | 30% | aspirazionale | Levesque & Vichesky 2006 |
| MySlim | 0.15 | 30% | aspirazionale | Swami & Tovée 2008 |
| MyMasculinity | 0.15 | 40% | assortativo forte | Bailey 1997 |
| MyAge (→myAgeNorm) | 0.07 | 30% | assort.+youth bias | Antfolk 2017 |
| MyHair | 0.05 | 40% | bipolare | Martins 2008 |
| MyHeight (→myHeightNorm) | 0.04 | 30% | aspirazionale | Valentova 2014 |
| MyEthnicity → `myEthBias` | molt. | — | post-distanza | OkCupid 2009 |
| MyPozStatus → `myPozBias` | molt. | — | post-distanza | CDC 2022 |

### Nel match SEXUAL cruising (4 + modulazione profilo)

| Variabile | Peso base | Modulazione | Tipo | Fonte |
|:---|:---|:---|:---|:---|
| MyBottomTop | 0.40 | ×(1+extremity×0.60) | complementare | Moskowitz 2008 |
| MySubDom | 0.28 | — | complementare | Bailey 1997 |
| MyKinkLevel | 0.17 | ×(1+KL×0.80) | speculare | Parsons 2010 |
| MyPenisSize | 0.15 | ×(1+bottomness×0.80) | aspir. BT-cond. | GMFA 2017 |

### Nel match APP single-pass (11 variabili, pesi normalizzati)

| Variabile | Peso norm. | Note |
|:---|:---|:---|
| BottomTop | 0.203 | domina — vincolante funzionale (Moskowitz 2008) |
| Muscle | 0.142 | |
| SubDom | 0.142 | |
| Beauty | 0.117 | |
| KinkLevel | 0.086 | |
| Slim / Masc / PenisSize | 0.076 | |
| Age | 0.036 | |
| Hair | 0.025 | |
| Height | 0.020 | |

### Nel profilo ma NON nel match

| Variabile | Motivazione esclusione |
|:---|:---|
| MyPolish | Nessun supporto come dimensione di mate evaluation |
| MyQueerness | Ridondante con MyMasculinity (r=−0.31) |
| MyBodyMod | Segnale identitario, non mate evaluation |
| MyBodyDisplay | UE5 vestiti, non mate evaluation |
| MySexActDepth | Costruzione interna senza supporto |
| MySexExtremity | Condiziona myKinkTypes ma non nel match diretto |
| MyOpenClosed | Futuro uso relazionale |
| MyRelDepth | Idem |
| MySelfEsteem | Derivata — BehaviorTree UE5 |

---

## Glossario

| Termine | Definizione |
|:---|:---|
| `IndividualProfile` | chi è il personaggio — prefisso `My` PascalCase |
| `MatingPreferences` | cosa cerca — prefisso `myDesired_` |
| `myTribeTag` | etichetta tribale — SOLO LOD e debug |
| `myKinkTypes` | lista flag kink attivi — Parsons 2010 |
| `myOut_visAttraction_cruise` | score visivo A→B — calcolato al momento |
| `myOut_sexAttraction_cruise` | score sessuale A→B — calcolato al momento |
| `myOut_attraction_app` | score combinato A→B — solo app |
| `pair_visMatched` | bool — mutual visual realizzato |
| `pair_visSexMatched` | bool — full match vis+sex realizzato (cruise) |
| `pair_fullMatched` | bool — match realizzato (app) |
| `myOut_visAttractionCount` | quante j l'NPC ha trovato visivamente attraenti |
| `myIn_visAttractionCount` | quanti lo trovano visivamente attraente |
| `my_visAttractionBalance` | myIn / myOut — asimmetria di mercato per-NPC |
| `my_visMatchCount_cruise` | contatore mutual visual accumulati |
| `my_visSexMatchCount_cruise` | contatore full match accumulati |
| `my_matchCount_app` | contatore match app accumulati |
| `my_visToSexFrustrationRate` | (visMatch − sexMatch) / visMatch |
| `myPop_visAttraction` | appeal visivo medio — media attrazioni ricevute |
| `myPop_sexAttraction` | desiderabilità sessuale, gated su mutual visual |
| `myPop_globalAttraction` | 0.60×vis + 0.40×sex (cruising) o unico (app) |
| `myWeightsVisual` | pesi individuali normalizzati — variabili visive |
| `myWeightsSexual` | pesi individuali normalizzati — variabili sessuali |
| `myWeightsSingle` | pesi individuali normalizzati — single-pass app |
| `myEthBias` | sensibilità individuale al bias etnico |
| `myPozBias` | sensibilità individuale al bias PozStatus |
| `my_visThreshold` | soglia visiva adattiva — solo Vergine |
| `my_sexThreshold` | soglia sessuale adattiva — solo Vergine |
| `pop_visAttractionPolarization` | Gini su myPop_visAttraction |
| `pairs_visCompatibilityRate` | % coppie con mutual visual |
| `pairs_visSexCompatibilityRate` | % coppie con full match cruise |
| `pairs_bottomTopIncompatibilityShare` | quota fallimenti per BT incompatibile |
| `pairs_kinkIncompatibilityShare` | quota fallimenti per kink mismatch |
| `pop_neverVisDesired_cruise` | % NPC mai trovati attraenti da nessuno |
| `pop_neverVisMatched_cruise` | % NPC senza mai mutual visual |
| `pop_neverSexMatched_cruise` | % NPC senza mai full match cruise |
| `pop_neverMatched_app` | % NPC senza mai match app |
| `MODE_CRUISING` | modalità two-step (incontro fisico) |
| `MODE_APP` | modalità single-pass (dating app) |
| `σ (sigma)` | selettività gaussiana — 0.13 cruising, 0.15 app |
