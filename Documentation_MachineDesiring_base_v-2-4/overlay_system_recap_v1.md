# MACCHINA DEL DESIDERIO — Overlay & Visualization System
## Implementation Recap v1.2

---

## STATO IMPLEMENTAZIONE (aggiornato 2026-03-24)

| Sezione | Stato | Note |
|---|---|---|
| §1.1 Threshold Architecture | ✅ IMPLEMENTATO | Modifiche rispetto al doc — vedi sotto |
| §1.2 AppAttractionMatrix | ✅ IMPLEMENTATO | Come da spec |
| §1.3 Cached Vectors | ✅ IMPLEMENTATO | Come da spec |
| §1.4 Population Scalar Metrics | ✅ IMPLEMENTATO | Due variabili rinominate — vedi sotto |
| §2 Overlay Architecture | ⬜ DA FARE | Nessun file creato |
| §3 Views | ⬜ DA FARE | Dipende da §2 |
| §4 Slider System | ⬜ DA FARE | Dipende da §2/§3 |
| §5 Variable Tooltips | ⬜ DA FARE | Dipende da §3 |

---

## 1. SYSTEM CHANGES — To implement in C++

### 1.1 Threshold Architecture (A_npcCharacter::InitThresholds)

Replace current hardcoded threshold values with the following structure for both Vis and Sex dimensions:

```cpp
// Global constants — U_populationSubsystem or global header
static constexpr float VIS_STANDARD_THRESHOLD = 0.35f;
static constexpr float SEX_STANDARD_THRESHOLD = 0.35f;
// NOTE: 0.35 is a calibrated artistic parameter. 
// TODO: verify against mate choice literature (Conroy-Beam, Buston & Emlen 2003)
// No empirical source found for separate vis/sex threshold values — kept identical.

// Per-NPC generated in InitThresholds() with deterministic seed = myId
// Vis
myVisStartThreshold = 0.35f + gauss(0.08, 0.03) clamp [0.03, 0.25]  → range [0.38, 0.60]
myVisCurrThreshold  = myVisStartThreshold
myVisMinThreshold   = 0.35f - gauss(0.12, 0.04) clamp [0.05, 0.25]  → range [0.10, 0.30]

// Sex
mySexStartThreshold = 0.35f + gauss(0.08, 0.03) clamp [0.03, 0.25]  → range [0.38, 0.60]
mySexCurrThreshold  = mySexStartThreshold
mySexMinThreshold   = 0.35f - gauss(0.12, 0.04) clamp [0.05, 0.25]  → range [0.10, 0.30]
```

**Invariant:** `myXxxMinThreshold < VIS/SEX_STANDARD_THRESHOLD < myXxxStartThreshold`

**Naming note:** `myVisStandardThreshold` replaces old `myVisMinThreshold = 0.40` which was an unfounded residual value.

> ✅ **IMPLEMENTATO** — `A_npcCharacter.cpp` / `U_populationSubsystem.h`
>
> **Modifiche rispetto alla spec:**
> - Costanti `VIS_STANDARD_THRESHOLD` e `SEX_STANDARD_THRESHOLD` dichiarate come `static constexpr float` in `U_populationSubsystem.h` (non in header separato).
> - `InitThresholds()` usa helper locale `InitThresholds_Gauss()` (Box-Muller) per evitare duplicazione. Non esposto come metodo di classe.
> - Seed offsets: VisStart=myId, VisMin=myId+500, SexStart=myId+1000, SexMin=myId+1500 — evita correlazione tra le quattro estrazioni per-NPC.
> - Vecchio `mySexMinThreshold = 0.40f` rimosso. Vecchio `myVisMinThreshold` = gauss(0.10,0.02) rimosso.

---

### 1.2 New Matrices in U_populationSubsystem

Add third N×N attraction matrix (~3.8MB):

```cpp
TArray<float> AppAttractionMatrix;  // MODE_APP single-pass, all 11 variables
```

**Comment to include in code:**
> AppAttractionMatrix uses CalcAppAttraction() — the MODE_APP single-pass calculation
> combining all 11 variables with literature-derived normalized weights.
> This is the recommended global attraction metric, replacing the arbitrary
> 0.60×vis + 0.40×sex formula. myPop_appAttraction is derived from this matrix.

Build in `BuildAttractionMatrices()` alongside existing vis/sex matrices.

> ✅ **IMPLEMENTATO** — `U_populationSubsystem.h/.cpp`
>
> `AppAttractionMatrix` aggiunta in `BuildAttractionMatrices()`, calcolata nello stesso loop vis/sex. Getter pubblico `GetAppScore(ObsId, TgtId)` esposto.

---

### 1.3 New Cached Vectors in U_populationSubsystem

Add to post-generation pipeline (all O(N) or O(N²), computed once):

```cpp
TArray<float> PopVisAttractionCache;    // myPop_visAttraction — already computed in
                                         // BuildMeshScaleCache as PopVis[], expose it
TArray<float> PopSexAttractionCache;    // myPop_sexAttraction
TArray<float> PopAppAttractionCache;    // myPop_appAttraction — from AppAttractionMatrix

TArray<int32> OutVisAttractionCount;    // myOut_visAttractionCount — uses STANDARD_THRESHOLD
TArray<int32> InVisAttractionCount;     // myIn_visAttractionCount  — uses STANDARD_THRESHOLD
TArray<float> VisAttractionBalance;     // my_visAttractionBalance = myIn / myOut
```

**Important:** `OutVisAttractionCount` and `InVisAttractionCount` use `VIS_STANDARD_THRESHOLD`
(0.35 fixed), NOT the per-NPC adaptive `myVisCurrThreshold`. Document this explicitly in code.

> ✅ **IMPLEMENTATO** — `U_populationSubsystem.h/.cpp`
>
> Tutti i vettori aggiunti in `BuildMeshScaleCache()` (che già calcolava `PopVis[]` internamente — ora esposto come `PopVisAttractionCache`). Getters pubblici: `GetPopVisAttraction`, `GetPopSexAttraction`, `GetPopAppAttraction`, `GetOutVisAttractionCount`, `GetInVisAttractionCount`, `GetVisAttractionBalance`.

---

### 1.4 New Population Scalar Metrics in U_populationSubsystem

Computed once post-generation. Expose as public getters:

```cpp
// Inequality
float PopVisAttractionPolarization;   // Gini coefficient on PopVisAttractionCache

// Pairs compatibility (computed scanning all N×N pairs at standard threshold)
float PairsVisCompatibilityRate;          // % pairs with mutual visual match
float PairsVisSexCompatibilityRate;       // % pairs with full vis+sex match
float PairsFullCompatibilityRateApp;      // % pairs with mutual app match
float PairsBottomTopIncompatibilityShare; // share of vis-matched pairs failing due to BT
                                           // BT dealbreaker: bt_distance > 0.60
float PairsKinkIncompatibilityShare;      // share failing due to kink mismatch
                                           // Kink dealbreaker: slider-configurable, default 0.80
                                           // condition: ONE direction sufficient (A→B OR B→A)

// Per-variable demand/supply (computed from Profiles array)
// For each variable X in {Muscle, Slim, Beauty, Masculinity, Hair, Age, BottomTop}:
float PopDemandSupplyRatio_Muscle;     // sum(myDesired_X) / sum(MyX)
float PopDemandSupplyRatio_Slim;
float PopDemandSupplyRatio_Beauty;
float PopDemandSupplyRatio_Masculinity;
float PopDemandSupplyRatio_Hair;
float PopDemandSupplyRatio_Age;
float PopDemandSupplyRatio_BottomTop;  // no complementary correction — same formula

// Per-variable desire gap (mean individual mismatch)
// gap_i = myDesired_X_i - MyX_i, then mean over population
float PopDesireGap_Muscle;
float PopDesireGap_Slim;
float PopDesireGap_Beauty;
float PopDesireGap_Masculinity;
float PopDesireGap_Hair;
float PopDesireGap_Age;
float PopDesireGap_BottomTop;          // no complementary correction — same formula
```

> ✅ **IMPLEMENTATO** — `U_populationSubsystem.h/.cpp`
>
> Calcolato in `BuildPopulationMetrics()`, chiamata dopo `BuildMeshScaleCache()` nel pipeline post-generazione.
>
> **Modifiche rispetto alla spec:**
> - `PairsVisSexCompatibilityRate` rinominata → **`PairsVisSexCompatibilityRate_cruise`**
> - `PairsBottomTopIncompatibilityShare` rinominata → **`PairsVisMatchedBottomTopIncompatibilityShare_cruise`**
> - BT incompatibility: usa `FMath::Abs(A.myDesired_BottomTop - B.MyBottomTop) > 0.60` in almeno una direzione (stessa formula del dealbreaker in `CalcSexualAttraction`). Il termine `bt_distance` del doc era un errore — non esiste come variabile separata.
> - Kink incompatibility: la spec diceva "ONE direction sufficient" — **modificato a ENTRAMBE le direzioni** su richiesta. Una coppia è kink-incompatibile solo se sia A→B che B→A superano la soglia 0.80.
> - `PairsFullCompatibilityRateApp` usa `VIS_STANDARD_THRESHOLD` come soglia app (0.35), non una soglia app separata.
> - Gini calcolato su `PopVisAttractionCache` con formula standard rank-based.

---

## 2. NEW SYSTEMS — Overlay Architecture

### 2.1 File Structure

```
Source/MachineDesiring/Overlay/
  U_overlaySubsystem.h / .cpp     // data aggregator, 3s timer
  UW_statBar.h / .cpp             // atomic reusable bar widget
  UW_populationPanel.h / .cpp     // population distributions panel
  UW_npcInspector.h / .cpp        // single NPC inspection panel
  UW_signalEvent.h / .cpp         // temporary signaling event popup
  UW_mainOverlay.h / .cpp         // main container, view switching
```

Add to `Build.cs` PrivateDependencyModuleNames:
```csharp
"UMG", "Slate", "SlateCore"
```

Add to `Build.cs` PublicIncludePaths:
```csharp
"MachineDesiring/Overlay"
```

### 2.2 Rendering Approach

**NativePaint override** on UUserWidget — draws lines, rectangles, text via FPaintContext.
No procedural textures, no additional assets. Blueprint subclass used only for layout positioning.

---

## 3. VIEWS SPECIFICATION

### VIEW 1 — Population Static
**Update:** never (post-generation fixed data)

#### 3.1 Continuous Variable Distributions
For each variable: **dual histogram** (My distribution + Desired distribution overlaid).
Slider below each histogram to adjust display threshold — default 0.35, reset button visible,
default marker always shown on slider track.

Variables, semantic labels (5-point scale), descriptions and display metrics:

---

**MyMuscle**
_Muscular development. Conditioned by age (piecewise decay: peak ~35yr, accelerated loss >60yr). Independent of role._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| atrophic | lean | average | muscular | hypertrophic | 0.30 |
Displays: dual histogram My+Desired · pop_demandSupplyRatio · pop_desireGap

---

**MySlim**
_Body leanness/fat distribution. Decreases with age, boosted by high muscle above threshold. NHANES-calibrated._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| obese | overweight | average | lean | emaciated | 0.48 |
Displays: dual histogram My+Desired · pop_demandSupplyRatio · pop_desireGap

---

**MyBeauty**
_Facial and overall aesthetic appeal. Strong age decay (−0.20×ageNorm). Captures structured facial symmetry and grooming._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| plain | below average | average | attractive | beautiful | 0.40 |
Displays: dual histogram My+Desired · pop_demandSupplyRatio · pop_desireGap

---

**MyMasculinity**
_Perceived gender expression on the masc/femme axis. Influenced by hair, queerness (negative), height (positive)._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| feminine | soft | neutral | masculine | hypermasculine | 0.62 |
Displays: dual histogram My+Desired · pop_demandSupplyRatio · pop_desireGap

---

**MyHair**
_Body hair amount. Increases with age, modulated by ethnicity (Asian −0.20, Middle Eastern +0.15). Preference is strongly polarized (masc4masc vs glabro subcultures)._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| hairless | sparse | medium | hairy | very hairy | 0.46 |
Displays: dual histogram My+Desired · pop_demandSupplyRatio · pop_desireGap

---

**MyAge (displayed as actual years, stored as norm (age−18)/82)**
_Chronological age. Strong assortative preference with youth bias. Oldest variable in terms of desire asymmetry._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| 18yr | 38yr | 49yr | 69yr | 100yr | 0.25 (≈38yr) |
Displays: dual histogram My+Desired · pop_demandSupplyRatio · pop_desireGap
Note: pop_desireGap_Age expected negative — population is older than desired.

---

**MyBottomTop**
_Sexual role on the bottom/top axis. Trimodal distribution: Bottom 28% / Versatile 50% / Top 22%. Complementary preference (desired = opposite of self)._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| bottom | soft bottom | versatile | soft top | top | 0.50 |
Displays: dual histogram My+Desired · pop_demandSupplyRatio · pop_desireGap

---

**MyQueerness**
_Visible gender non-conformity and queer cultural expression. Bimodal distribution (passing vs. openly queer). Negatively correlated with MyMasculinity. Not in match calculation — profile/LOD only._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| straight-passing | mostly passing | ambiguous | queer-coded | flamboyant | ~0.46 (bimodal) |
Displays: single histogram (no Desired counterpart)

---

**MyPolish**
_Personal grooming, style refinement, social presentation. Independent variable. Not in match calculation._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| unkempt | rough | average | groomed | refined | 0.44 |
Displays: single histogram (no Desired counterpart)

---

**MyBodyDisplay**
_Degree to which the NPC exposes their body (clothing level, exhibitionism of physical form). Used for UE5 clothing LOD. Not the same as erotic exhibitionism (see MySexExtremity). Not in match calculation._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| fully covered | conservative | moderate | skin-forward | maximally exposed | ~0.22 |
Displays: single histogram (no Desired counterpart)

---

**MySexActDepth**
_Preference for intensity and depth of sexual experience, beyond role or kink. High values indicate preference for immersive, prolonged, or emotionally intense encounters. Not in match calculation._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| superficial | light | moderate | deep | intense | ~0.38 |
Displays: single histogram (no Desired counterpart)

---

**MySexExtremity**
_General sexual extremity — how far outside mainstream the NPC's sexual interests extend. Conditions myKinkTypes assignment. Not a direct match variable but shapes the kink flag profile._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| vanilla | mild | moderate | edgy | extreme | ~0.32 |
Displays: single histogram (no Desired counterpart)

---

**MySelfEsteem**
_Relational self-esteem derived from physical and social attributes (sociometer model). Formula: 0.21 + Beauty×0.20 + Muscle×0.15 + Slim×0.10 + Masculinity×0.08 + PenisSize×0.12 − AgeNorm×0.10 + noise. Used for BehaviorTree in future development._
| 0.0 | 0.25 | 0.5 | 0.75 | 1.0 | Pop Mean |
|---|---|---|---|---|---|
| very low | low | average | confident | very high | ~0.48 |
Displays: single histogram (no Desired counterpart)

---

Each histogram also displays:
- `pop_demandSupplyRatio_X` — single number with color coding (>1 red = undersupply, <1 blue = oversupply, ~1 green)
- `pop_desireGap_X` — single number with sign (positive = population desires more than it has)

**Tooltip on hover (popup):**
Each histogram shows a popup with variable name, description, semantic scale, formula, and interpretation guide.

#### 3.2 Categorical Distributions
**Horizontal barchart** with label + %.

| Variable | Type | Notes |
|---|---|---|
| MyEthnicity | barchart | 6 categories: White/Latino/Black/Asian/MiddleEastern/Mixed |
| MyPozStatus | barchart | 3 categories: None/PozUndet/Poz |
| myTribeTag | barchart | 14 categories + Average |

#### 3.3 Appeal Distribution
**What it shows:** how desirable each NPC is in the eyes of the entire population. These are emergent metrics — not inputs but outcomes of the attraction system.

Three separate histograms, one per metric:

**myPop_visAttraction**
_Mean visual attraction received from all other NPCs. Measures how visually desirable this NPC is to the population as a whole. Computed from VisAttractionMatrix column means. High Gini = strong inequality in visual desirability._
Displays: histogram of per-NPC values across population + Gini coefficient value overlay.

**myPop_sexAttraction**
_Mean sexual attraction received from all other NPCs. Computed from SexAttractionMatrix column means. Independent of visual — an NPC can be sexually compatible with many but visually attractive to few._
Displays: histogram + Gini.

**myPop_appAttraction**
_Mean app-mode (single-pass) attraction received. Uses CalcAppAttraction() — all 11 variables with literature-derived normalized weights (BottomTop dominant at 0.203). This is the recommended global desirability metric, replacing the deprecated arbitrary 0.60×vis + 0.40×sex formula. Higher = more universally desirable across both dimensions simultaneously._
Displays: histogram + Gini.

**Slider:** vis/sex/app threshold sliders (independent, default 0.35) affect count-based metrics but NOT these mean-based appeal histograms.

#### 3.4 Pairs Compatibility Metrics
**What it shows:** structural compatibility rates across all possible NPC pairs in the population. Reveals how much of the potential match space is accessible vs. blocked by specific incompatibilities.

**Stacked barchart:**

| Metric | Description | How to read |
|---|---|---|
| pairs_visCompatibilityRate | % of all possible pairs with mutual visual attraction ≥ threshold | Higher = more visual matches available in population |
| pairs_visSexCompatibilityRate | % of all possible pairs with mutual visual AND sexual match | Much lower than vis — sexual compatibility is a strong filter |
| pairs_fullCompatibilityRateApp | % of all possible pairs with mutual app-mode match | Single-pass — typically higher than cruise full match |

**Failure breakdown barchart:**

| Metric | Description | How to read |
|---|---|---|
| pairs_bottomTopIncompatibilityShare | Share of vis-matched pairs failing specifically due to BT incompatibility (bt_distance > 0.60) | High = role market is a major structural bottleneck |
| pairs_kinkIncompatibilityShare | Share failing due to kink mismatch (one direction kl_distance > slider threshold) | High = kink diversity creates fragmentation |

**Slider — kink incompatibility threshold:** default 0.80, reset button, default marker on track.
Condition: ONE direction sufficient (if A→B OR B→A exceeds threshold, pair counted as kink-incompatible).

**Slider — vis/sex/app threshold (independent):** affects all pairs_* counts in real time.
Recomputation operates on cached matrices only — attraction values are NOT recalculated.

#### 3.5 Attraction Count Distribution
**What it shows:** how many NPCs each individual finds attractive (outgoing) vs. how many find them attractive (incoming), and the ratio between the two. Reveals market asymmetry at the individual level.

**myOut_visAttractionCount + myIn_visAttractionCount**
_Per-NPC counts of outgoing and incoming visual attractions above threshold. Dual histogram overlaid._
- myOut = how selective this NPC is (low = very selective, high = finds many attractive)
- myIn = how desirable this NPC is by headcount (low = few find them attractive)
Displays: dual histogram overlaid, two colors.

**my_visAttractionBalance**
_myIn_visAttractionCount / myOut_visAttractionCount. Market position ratio per NPC._
- >1.0 = desired by more than they desire (favorable position)
- <1.0 = desires more than receives (structural frustration)
- =1.0 = symmetric market position
Displays: separate histogram with vertical reference line at 1.0.

**Slider — vis attraction threshold (independent from sex/app):** default 0.35, reset button.
Recomputes myOut, myIn, balance in real time from cached VisAttractionMatrix.

---

### VIEW 2 — Runtime Population
**Update:** every 3 seconds via timer in U_overlaySubsystem.

| Metric | Visualization | Notes |
|---|---|---|
| FSM states (Cruising/Mating/Latency) | horizontal barchart live | 3 values |
| Cruising substates (RW/Idle/Mating/Target) | horizontal barchart live | 4 values |
| NPCs signaling | large single number + % | |
| NPCs with myDesiredRank > 0 | large single number + % | |
| Active mating pairs | large single number | |
| Mean myVisCurrThreshold | number + distance from standard | shows population frustration pressure |
| NPCs below myVisStandardThreshold | count + % | key frustration indicator |

---

### VIEW 3 — NPC Inspector
**Activation:** click on NPC in scene OR selection from NPC list in overlay UI.
**Update:** on demand (on selection change).

| Section | Content | Visualization |
|---|---|---|
| Identity | myId, myTribeTag, MyEthnicity, MyPozStatus, MySafetyPractice, myAgeNorm, myHeightNorm | key-value list |
| Body | MyAge, MyHeight, MyMuscle, MySlim, MyBeauty, MyMasculinity, MyHair, MyBodyMod, MyBodyDisplay | key-value + mini bar |
| Sexual | MyBottomTop, MySubDom, MyKinkLevel, MyPenisSize, MySexExtremity, MySexActDepth | key-value + mini bar |
| Psych | MySelfEsteem, MyOpenClosed, MyRelDepth, MyQueerness, MyPolish | key-value + mini bar |
| Kink flags | myKinkTypes active flags | tag list |
| Thresholds | myVisStartThreshold, myVisCurrThreshold, myVisMinThreshold | three-point bar (start/curr/min) |
| | mySexStartThreshold, mySexCurrThreshold, mySexMinThreshold | three-point bar |
| Appeal | myPop_visAttraction, myPop_sexAttraction, myPop_appAttraction | three numbers + percentile rank in population |
| Desired rank | myDesiredRank top 5 with visScore per entry | ranked list |
| Social memory | count by state (Desired/NotYetDesired/GaveUp/Lost/SexBlocked) | mini barchart |
| State | myCurrentState + myCruisingSubState | label |
| Attraction counts | myOut_visAttractionCount, myIn_visAttractionCount, my_visAttractionBalance | numbers |

---

### VIEW 4 — Signal Event Popup
**Activation:** triggered from A_npcController::ExchangeSignals when handshake occurs.
**Duration:** visible for 3 seconds, then fades.
**Position:** screen overlay, top or side panel.

| Content | Visualization |
|---|---|
| NPC pair ID: A ↔ B | header |
| myOut_signalValue / myIn_signalValue for both | two bars |
| Rank of B in A's desiredRank (and vice versa) | number |
| visScore A→B and B→A | two values |
| Current signal totals (cumulative) | numbers |

---

## 4. SLIDER SYSTEM — General Rules

All sliders follow these rules:
- Default value always marked with a visible tick/notch on the track
- Reset button always present and visible
- Label shows current value numerically
- Recomputation is real-time on drag (operates on cached matrices, not recalculating attractions)
- Sliders for vis and sex thresholds are **independent**

| Slider | Default | Affects |
|---|---|---|
| Vis attraction threshold | 0.35 | myOut/myIn counts, pairs_vis metrics, my_visAttractionBalance |
| Sex attraction threshold | 0.35 | pairs_visSex metrics |
| App attraction threshold | 0.35 | pairs_fullCompatibilityRateApp |
| Kink incompatibility threshold | 0.80 | pairs_kinkIncompatibilityShare |

---

## 5. VARIABLE TOOLTIPS — Popup Descriptions

Each chart/metric in the UI shows a popup on hover with:
1. Variable name (C++ convention)
2. One-line description
3. How to read it (what high/low values mean)
4. Formula summary

Key tooltips to implement:

**myPop_appAttraction**
> "Mean app-mode attraction received from all other NPCs. Uses CalcAppAttraction() — MODE_APP single-pass with all 11 literature-derived normalized weights (BottomTop dominant at 0.203). This replaces the previous arbitrary 0.60×vis + 0.40×sex formula. Higher = more universally desirable across both visual and sexual dimensions."

**pop_demandSupplyRatio_X**
> "sum(myDesired_X across population) / sum(MyX across population). >1 = population desires more of this trait than exists (undersupply). <1 = trait more abundant than desired (oversupply). =1 = perfect balance."

**pop_desireGap_X**
> "Mean of (myDesired_X - MyX) across all NPCs. Positive = population on average wants more of this trait than it possesses. Negative = population possesses more than it desires."

**my_visAttractionBalance**
> "myIn_visAttractionCount / myOut_visAttractionCount. >1 = this NPC is desired by more people than it desires (favorable market position). <1 = desires more than receives (structural frustration). Computed at standard threshold 0.35."

**VIS_STANDARD_THRESHOLD**
> "Reference threshold 0.35 — calibrated artistic parameter. TODO: verify empirical basis in mate choice literature. Used for all population-level count metrics. Individual NPCs start above this (myVisStartThreshold) and may drop below it under frustration pressure."

**pop_visAttractionPolarization**
> "Gini coefficient on myPop_visAttraction distribution. 0 = all NPCs receive equal visual attention. 1 = all attention concentrated on one NPC. Expected ~0.313 at N=700."

---

## 6. OPEN TODOS

| Priority | Item |
|---|---|
| HIGH | Verify 0.35 threshold against mate choice literature (Conroy-Beam, Buston & Emlen 2003) |
| HIGH | Expose PopVis[] from BuildMeshScaleCache as myPop_visAttraction instead of discarding it |
| MED | Verify pairs_kinkIncompatibilityShare direction condition (one vs both) — set to ONE direction |
| MED | myPop_globalAttraction formally removed — replaced by myPop_appAttraction. Update all docs |
| LOW | In-world 3D per-NPC infographic (future, post debug phase) |
| LOW | Public audience interactive mode (future) |

---

## 7. REMOVED / DEPRECATED

| Item | Reason |
|---|---|
| `myPop_globalAttraction` (0.60×vis + 0.40×sex) | Arbitrary weights, no empirical source. Replaced by `myPop_appAttraction` |
| `mySexMinThreshold = 0.40` hardcoded | Unfounded residual. Replaced by dynamic gauss generation |
| Separate vis/sex standard threshold values | No literature source for difference. Both = 0.35 |

