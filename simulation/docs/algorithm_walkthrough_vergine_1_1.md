# ALGORITHM WALKTHROUGH — Vergine Totale v1.1

Spiegazione discorsiva dell'algoritmo di apprendimento del desiderio.

---

## ARCHITETTURA GENERALE

**Vergine Totale** è un modello di apprendimento adattivo dove **ogni NPC scopre cosa vuole attraverso l'esperienza**. A differenza di v2.3 (target fissi calcolati all'inizio), qui i target di desiderio (`myDesired_X`) si evolvono in base a:

1. **Esperienza diretta** — chi matcha con me sposta i miei target
2. **Osservazione sociale** — imparo cosa funziona guardando i match altrui
3. **Campionamento popolazione** — capisco cosa esiste nella realtà
4. **Prior innati** — parto con bias verso "attraente" generico

---

## COEFFICIENTI — Le 4 Tensioni

Il desiderio emerge dal bilanciamento di 4 forze, ciascuna rappresentata da un coefficiente adattivo:

### 1. ASPIRATION (miglioramento verso target)

**Cosa rappresenta:** Quanto voglio "meglio di me stesso" verso un target aspirazionale.

**Range:** [0.50, 0.80] → ceiling a 0.75 dopo successi

**Inizializzazione:**
```python
aspiration = N(0.65, 0.08) clipped [0.50, 0.80]
```

**Interpretazione:**
- `aspiration = 0.50` → metà tra me e target
- `aspiration = 0.65` → 65% verso target, 35% verso me
- `aspiration = 0.75` → 75% verso target (ceiling)

**Dinamica:**
- **Fallimenti** → aspiration scende (×0.96 a ×0.99)
- **Successi** → aspiration sale (×1.01)

**Fonte teorica:** Hitsch 2010 — aspirazione ottimista pre-esperienza, calibrazione con rejection.

---

### 2. SELFMIRROR (accettazione simile a me)

**Cosa rappresenta:** Quanto accetto persone simili a me stesso.

**Range:** [0.20, 0.50]

**Inizializzazione:**
```python
selfMirror = N(0.35, 0.08) clipped [0.20, 0.50]
```

**Interpretazione:**
- `selfMirror = 0.20` → bassa accettazione similarità (voglio diverso da me)
- `selfMirror = 0.35` → moderata accettazione similarità
- `selfMirror = 0.50` → alta accettazione similarità

**Dinamica:**
- **Fallimenti** → selfMirror sale (×1.02 a ×1.04)
- **Successi** → selfMirror scende lievemente (×0.99)

**Relazione con aspiration:**
- NON reciproci (indipendenti)
- Ma dinamica anticorrelata emergente
- Correlazione attesa r ≈ −0.5 (non −1.0)

**Fonte teorica:** Kalick & Hamilton 1986 — assortative mating emerge da calibrazione, non è innato.

---

### 3. CULTURALPULL (conformità all'ideale osservato)

**Cosa rappresenta:** Quanto peso l'ideale culturale che osservo nei match di successo.

**Range:** [0, 0.30]

**Inizializzazione:**
```python
# Round 0: 0.0 (non conosco ancora ideale)
# Round 1: 0.15 base + 0.10 × match_rate_round0
```

**Calcolo ideale osservato:**
```python
# Solo TOP 30% match per attraction ricevuta
top_matches = top_30_percent_by_visual_attraction(matched)
observedCulturalIdeal[X] = mean([profiles[j][X] for j in top_matches])

# Update con learning rate 0.10
ideal_new = old + 0.10 × (top_mean - old)
```

**Interpretazione:**
- `culturalPull = 0.0` → ignoro ideale culturale
- `culturalPull = 0.15` → 15% peso su ideale
- `culturalPull = 0.25` → 25% peso su ideale

**Dinamica:**
- **Full match con partner allineato a ideale** → culturalPull sale (+0.01 × alignment)
- **Full match con partner lontano da ideale** → culturalPull scende (×0.99)

**Fonte teorica:** Henrich & Boyd 2001 — cultural transmission via prestige bias (imparo dai "vincenti").

---

### 4. MARKETAWARENESS (adattamento alla realtà)

**Cosa rappresenta:** Quanto mi adatto alla media della popolazione reale.

**Range:** [0, 0.30]

**Inizializzazione:**
```python
marketAwareness = 0.0  # round 0
```

**Crescita:** Incremento progressivo ogni interazione (+0.002 a +0.010 secondo scenario).

**Calcolo stima popolazione:**
```python
# Round 0: bootstrap da primo batch
estimatedPopMean[X] = mean(batch_visto)

# Round 1+: media ponderata esatta
estimatedPopMean[X] = (old×n_prev + batch_mean×n_curr) / (n_prev+n_curr)
```

**Interpretazione:**
- `marketAwareness = 0.0` → ignoro cosa esiste nella popolazione
- `marketAwareness = 0.10` → 10% peso su realtà
- `marketAwareness = 0.20` → 20% peso su realtà

**Dinamica:** Cresce monotonicamente con esperienza (tutti gli eventi incrementano).

**Fonte teorica:** Todd et al. 2007 — mate preferences calibrate to market availability.

---

## FORMULA myDesired_X — Calcolo Completo

### STEP 1: Calcola "better than self"

```python
better_than_self = MyTrait + aspiration × (1 - MyTrait)
```

**Esempio:** MyMuscle=0.30, aspiration=0.65
```
better = 0.30 + 0.65×(1-0.30) = 0.30 + 0.455 = 0.755
```

**Interpretazione:** "Voglio muscolo a livello 0.755" (gap +0.455 assoluto).

---

### STEP 2: Componente core (aspiration + selfMirror)

```python
total_core = aspiration + selfMirror
core_desired = (aspiration × better_than_self + selfMirror × MyTrait) / total_core
```

**Esempio:** aspiration=0.65, selfMirror=0.35
```
total = 0.65 + 0.35 = 1.00
core = (0.65×0.755 + 0.35×0.30) / 1.00
     = (0.491 + 0.105) / 1.00
     = 0.596
```

**Interpretazione:** Bilanciamento tra "voglio meglio" (0.755) e "accetto simile" (0.30).

---

### STEP 3: Formula completa (+ ideale + mercato)

**Round 0 (culturalPull=0, marketAwareness=0):**
```python
myDesired_Muscle = core_desired  # solo componente innata
```

**Round 1+ (con culturalPull, marketAwareness):**
```python
myDesired_Muscle = (
    core_desired +
    culturalPull × observedCulturalIdeal['Muscle'] +
    marketAwareness × estimatedPopMean['Muscle']
) / (1.0 + culturalPull + marketAwareness)
```

**Esempio Round 20:**
```
core = 0.596
culturalPull = 0.18
marketAwareness = 0.15
observedIdeal = 0.68 (top 30% match osservati)
estimatedPop = 0.44 (media reale)

myDesired_Muscle = (0.596 + 0.18×0.68 + 0.15×0.44) / (1.0 + 0.18 + 0.15)
                 = (0.596 + 0.122 + 0.066) / 1.33
                 = 0.784 / 1.33
                 = 0.589
```

---

### CASO SPECIALE: BottomTop (complementarità innata)

```python
if var == 'BottomTop':
    better_than_self = 1.0 - MyBottomTop  # inverso
    core_desired = 0.70 × better_than_self + 0.30 × MyBottomTop
    # Peso fisso complementarità 70%, self 30%
```

**Razionale:** Un bottom "sa" di volere un top (innato), ma con lieve tolleranza vers.

**Fonte:** Moskowitz & Hart 2011 — complementarità BT è funzionale, non appresa.

---

## MATRICE EVENTI — 7 Scenari

Ogni interazione rientra in uno di 7 scenari, ciascuno con effetti specifici su coefficienti, `myDesired_X`, e soglie.

### V1 — MUTUA ESCLUSIONE VISIVA

**Condizione:**
```python
vis_ij < my_visThreshold AND vis_ji < their_visThreshold
```

**Effetti:**
```python
aspiration *= 0.98
selfMirror *= 1.02
marketAwareness += 0.003
my_visThreshold *= (1 - 0.020)  # decay
my_visFrustration += 0.12
```

**Interpretazione:** "Non ci piacciamo. Devo abbassare leggermente aspirazioni e accettare più simile."

**Fonte:** Todd 2007 — mutual low interest → modest standard lowering.

---

### V2 — RICEVO ATTRAZIONE NON RICAMBIATA

**Condizione:**
```python
vis_ij < my_visThreshold AND vis_ji >= their_visThreshold
```

**Effetti:**
```python
aspiration *= 0.99
selfMirror *= 1.01
marketAwareness += 0.002
my_isolationFrustration *= 0.95  # validazione esterna

# Rinforzo debole verso tratti di j
for var in ALL_VARS:
    myDesired[var] += 0.03 × (j_trait - myDesired[var])
```

**Interpretazione:** "Quella persona mi ha desiderato. Forse quel tipo di profilo è raggiungibile."

**Fonte:** Li 2002 — external validation modulates standards.

---

### V3 — ESPRIMO ATTRAZIONE, VENGO RESPINTO

**Condizione:**
```python
vis_ij >= my_visThreshold AND vis_ji < their_visThreshold
```

**Effetti:**
```python
aspiration *= 0.97
selfMirror *= 1.03
marketAwareness += 0.004
my_visFrustration += 0.10

# Allontano da tratti di j
for var in ALL_VARS:
    myDesired[var] += -0.02 × (j_trait - myDesired[var])
```

**Interpretazione:** "Volevo quella persona ma mi ha rifiutato. Quel livello è fuori portata — abbasso target."

**Fonte:** Kenrick 1993 — rejection calibrates mate value.

---

### S1 — MUTUAL VISUAL, MUTUAL NO SEXUAL

**Condizione:**
```python
vis mutual AND sex_ij < my_sexThreshold AND sex_ji < their_sexThreshold
```

**Effetti:**
```python
aspiration *= 0.96
selfMirror *= 1.04
marketAwareness += 0.005
my_visThreshold += 0.008  # segnale positivo visual
my_sexThreshold *= (1 - 0.015)  # decay
my_sexFrustration += 0.18
```

**Interpretazione:** "Ci piacciamo visivamente ma non funzioniamo sessualmente. Devo abbassare pretese sessuali."

**Fonte:** Moskowitz & Hart 2011 — visual ≠ sexual compatibility.

---

### S2a — MUTUAL VISUAL, IO SÌ SEX, LORO NO

**Condizione:**
```python
vis mutual AND sex_ij >= my_sexThreshold AND sex_ji < their_sexThreshold
```

**Effetti:**
```python
aspiration *= 0.97
selfMirror *= 1.03
marketAwareness += 0.004
my_visThreshold += 0.008
my_sexThreshold *= 0.988
my_sexFrustration += 0.15

# Allontano da tratti SESSUALI di j
for var in SEXUAL_VARS:  # BottomTop, SubDom, KinkLevel, PenisSize
    myDesired[var] += -0.03 × (j_trait - myDesired[var])
```

**Interpretazione:** "Mi piaceva sessualmente ma loro no. Devo allontanare target sessuale da quel profilo."

**Fonte:** Moskowitz 2008 — sexual incompatibility feedback is specific.

---

### S2b — MUTUAL VISUAL, LORO SÌ SEX, IO NO

**Condizione:**
```python
vis mutual AND sex_ij < my_sexThreshold AND sex_ji >= their_sexThreshold
```

**Effetti:**
```python
aspiration *= 0.98
selfMirror *= 1.02
marketAwareness += 0.003
my_visThreshold += 0.008
my_sexFrustration += 0.08

# Rinforzo debole verso tratti SESSUALI di j
for var in SEXUAL_VARS:
    myDesired[var] += 0.02 × (j_trait - myDesired[var])
```

**Interpretazione:** "Loro mi desideravano sessualmente anche se io no. Forse dovrei considerare quel profilo."

**Fonte:** Li 2002 — external validation anche su dimensione sessuale.

---

### S3 — FULL MATCH (SUCCESSO)

**Condizione:**
```python
vis mutual AND sex mutual
```

**Effetti:**
```python
aspiration *= 1.01
aspiration = min(0.75, aspiration)  # ceiling
selfMirror *= 0.99
marketAwareness += 0.010

# culturalPull adjustment
alignment = calculate_partner_alignment_with_ideal(j)
if alignment > 0.70:
    culturalPull += 0.01 × alignment
else:
    culturalPull *= 0.99

# Rinforzo FORTE verso TUTTI i tratti di j
for var in ALL_VARS:
    myDesired[var] += 0.12 × (j_trait - myDesired[var])

# Soglie salgono
my_visThreshold += 0.025
my_sexThreshold += 0.025
my_visThreshold = min(0.95, my_visThreshold)
my_sexThreshold = min(0.95, my_sexThreshold)

# Frustrazione decade
my_visFrustration *= 0.80
my_sexFrustration *= 0.80
my_isolationFrustration *= 0.70

# Registra match
my_fullMatchCount += 1
```

**Interpretazione:** "SUCCESSO! Questa persona funziona per me. Voglio altre persone simili (o meglio). Alzo pretese."

**Fonti:**
- Hitsch 2010 — successful match → standards increase
- Luo 2017 — partner traits become template
- Conroy-Beam 2019 — mate value recalibration upward after success

---

## PIPELINE COMPLETA — Round-by-Round

### INIZIALIZZAZIONE (Round 0 setup)

```python
# 1. Genera profili
profiles = [generate_profile() for _ in range(N)]
prefs = [generate_preferences(p) for p in profiles]

# 2. Inizializza coefficienti
for i in range(N):
    aspiration[i] = N(0.65, 0.08) clipped [0.50, 0.80]
    selfMirror[i] = N(0.35, 0.08) clipped [0.20, 0.50]
    culturalPull[i] = 0.0
    marketAwareness[i] = 0.0
    
    # Stime iniziali
    observedCulturalIdeal[i] = {var: 0.50 for var in ALL_VARS}
    estimatedPopMean[i] = {var: 0.50 for var in ALL_VARS}
    
    # Target iniziali (calcolati con coefficienti round 0)
    for var in ALL_VARS:
        myDesired[i][var] = _calculate_myDesired(var, state[i], profiles[i])
```

---

### OGNI ROUND

#### FASE 1: Subsample batch (30 persone)

```python
batch_indices = random_choice(N, size=30, replace=False)
```

#### FASE 2: Calcola attrazioni visuali (tutti vs tutti)

```python
for i in batch:
    for j in batch:
        if i == j: continue
        vis_ij = calc_visual_attraction_learned(profiles[i], state[i], prefs[i], profiles[j])
        vis_cache[(i,j)] = vis_ij
        
        # Aggiorna received sum (per floor)
        state[j]['my_receivedAttractionSum'] += vis_ij
        state[j]['my_receivedAttractionN'] += 1
```

#### FASE 3: Identifica mutual visual, calcola attrazioni sessuali

```python
for i, j in batch × batch:
    i_vis_j = (vis_cache[i,j] >= state[i]['my_visThreshold'])
    j_vis_i = (vis_cache[j,i] >= state[j]['my_visThreshold'])
    
    if i_vis_j AND j_vis_i:
        # Mutual visual → calcola sexual
        sex_ij = calc_sexual_attraction_learned(...)
        sex_ji = calc_sexual_attraction_learned(...)
        sex_cache[(i,j)] = sex_ij
        sex_cache[(j,i)] = sex_ji
        
        # Conta vis match
        state[i]['my_visMatchCount'] += 1
        state[j]['my_visMatchCount'] += 1
```

#### FASE 4: Classifica eventi e applica effetti

```python
for i in batch:
    for j in batch:
        if i == j: continue
        
        scenario = classify_scenario(i, j, vis_cache, sex_cache, state)
        # scenario ∈ {V1, V2, V3, S1, S2a, S2b, S3}
        
        apply_scenario_effects(i, j, scenario, state, profiles)
        # Aggiorna: aspiration, selfMirror, culturalPull, marketAwareness,
        #           myDesired_X, soglie, frustrazione
```

#### FASE 5: Aggiorna stima popolazione

```python
for i in batch:
    n_prev = state[i]['my_encounterCount']
    n_curr = len(batch) - 1  # escludo me stesso
    
    for var in ALL_VARS:
        batch_mean = mean([profiles[j][var] for j in batch if j != i])
        
        if n_prev == 0:
            # Bootstrap
            estimatedPopMean[i][var] = batch_mean
        else:
            # Media ponderata esatta
            old_mean = estimatedPopMean[i][var]
            new_mean = (old_mean × n_prev + batch_mean × n_curr) / (n_prev + n_curr)
            estimatedPopMean[i][var] = new_mean
    
    state[i]['my_encounterCount'] += n_curr
```

#### FASE 6: Aggiorna ideale osservato (top 30%)

```python
for i in batch:
    # Chi ha matchato?
    matched = [j for j in batch if j in state[i]['my_matchedPartners']]
    
    if len(matched) < 3: continue  # troppo pochi
    
    # Ordina per attraction ricevuta
    ranked = sorted(matched, key=lambda j: state[j]['receivedAttractionMean'], reverse=True)
    
    # Top 30%
    n_top = max(1, int(len(matched) × 0.30))
    top_matches = ranked[:n_top]
    
    # Ideale = media top
    for var in ALL_VARS:
        new_ideal = mean([profiles[j][var] for j in top_matches])
        old_ideal = observedCulturalIdeal[i][var]
        
        # Update con LR 0.10
        observedCulturalIdeal[i][var] = old_ideal + 0.10 × (new_ideal - old_ideal)
```

#### FASE 7: Ricalcola TUTTI i myDesired_X

```python
for i in batch:
    for var in ALL_VARS:
        myDesired[i][var] = _calculate_myDesired(var, state[i], profiles[i])
```

#### FASE 8: Aggiorna floor e SelfEsteem

```python
for i in batch:
    # Floor da received mean
    recv_vis_mean = receivedAttractionSum[i] / max(1, receivedAttractionN[i])
    my_visFloor[i] = max(0.02, recv_vis_mean × 0.75)
    
    recv_sex_mean = receivedSexAttractionSum[i] / max(1, receivedSexAttractionN[i])
    my_sexFloor[i] = max(0.02, recv_sex_mean × 0.75)
    
    # SelfEsteem sociometro
    if fullMatchCount_round > 0:
        outcome_signal = 0.68
    elif visMatchCount_round > 0:
        outcome_signal = 0.51
    else:
        outcome_signal = 0.35
    
    MySelfEsteem_dynamic += 0.08 × (outcome_signal - MySelfEsteem_dynamic)
    MySelfEsteem_dynamic += anchor_effect × (MySelfEsteem_static - MySelfEsteem_dynamic)
```

---

### INIZIALIZZAZIONE culturalPull (dopo Round 0)

```python
if round_num == 0:
    for i in range(N):
        match_rate = fullMatchCount[i] / len(batch)
        culturalPull[i] = 0.15 + 0.10 × match_rate
        # Range: [0.15, 0.25]
```

---

## METRICHE DI CONVERGENZA

### Coefficienti — Distribuzione Finale

**aspiration (round 50):**
- Bottom tier (0 match): μ ≈ 0.32, range [0.20, 0.45]
- Mid tier (1-3 match): μ ≈ 0.55, range [0.45, 0.65]
- Top tier (5+ match): μ ≈ 0.72, range [0.65, 0.75 ceiling]

**selfMirror (round 50):**
- Bottom tier: μ ≈ 0.58, range [0.45, 0.70]
- Mid tier: μ ≈ 0.38, range [0.30, 0.50]
- Top tier: μ ≈ 0.28, range [0.20, 0.35]

**Correlazione aspiration × selfMirror:** r ≈ −0.52 (anticorrelati ma non reciproci perfetti)

**culturalPull (round 50):**
- μ ≈ 0.18, range [0.15, 0.25]

**marketAwareness (round 50):**
- μ ≈ 0.22, range [0.15, 0.30]

---

### myDesired_X — Convergenza vs v2.3

**Correlazione tra learned e static targets:**

| Variabile | r (learned, v2.3) | Interpretazione |
|:---|:---:|:---|
| Muscle | +0.45 | Convergenza moderata (appreso vs innato) |
| Beauty | +0.68 | Alta convergenza (universale) |
| BottomTop | +0.82 | Alta convergenza (complementarità innata) |
| Age | +0.38 | Bassa convergenza (appreso da esposizione) |
| PenisSize | +0.51 | Moderata (funzione ruolo sessuale) |

**Interpretazione:** Tratti con componente innata/universale convergono di più verso v2.3. Tratti culturali/appresi divergono di più.

---

## DIFFERENZE DA v2.3 (STATIC)

| Aspetto | v2.3 | Vergine Totale v1.1 |
|:---|:---|:---|
| **myDesired_X** | Fisso (calcolato round 0) | Adattivo (ricalcolato ogni round) |
| **Coefficienti** | Impliciti in formula statica | Espliciti e adattivi |
| **Ideale culturale** | Da letteratura (input) | Osservato da top 30% match (emergente) |
| **Stima popolazione** | Da POP_MEAN (input) | Campionata da incontri |
| **Soglie** | In Vergine v1.0 erano adattive | Anche qui adattive (ereditate) |
| **Pesi** | Fissi individuali | Fissi individuali (stesso) |

---

## NOTE IMPLEMENTATIVE

### Ordine Esecuzione Critico

1. **Calcola attrazioni** PRIMA di aggiornare coefficienti
2. **Aggiorna coefficienti** DOPO tutti gli eventi
3. **Aggiorna stime** (popMean, ideal) DOPO coefficienti
4. **Ricalcola myDesired_X** DOPO aggiornamento stime
5. **Soglie/floor/SelfEsteem** per ultimi

### Evitare Feedback Loops

- **estimatedPopMean** non influenza match corrente (solo round successivo)
- **observedIdeal** da TOP 30% evita contaminazione con media
- **culturalPull** adjustment basato su allineamento evita rinforzo spurio

### Stabilità Numerica

- Tutti i coefficienti hanno floor (aspiration ≥ 0.50, selfMirror ≥ 0.20)
- Normalizzazione (asp+sm) / (asp+sm) evita overflow
- Clamp myDesired_X in [0.01, 0.99]

---

## FONTI PRINCIPALI

- **Todd et al. 2007** — Speed-dating, adjustment patterns
- **Hitsch et al. 2010** — Online dating, standards evolution
- **Kalick & Hamilton 1986** — Assortative mating simulation
- **Luo 2017** — Partner traits as template
- **Moskowitz & Hart 2011** — BT complementarità, visual/sexual mismatch
- **Conroy-Beam 2019** — Assortative mating evolution
- **Li et al. 2002** — External validation, necessities/luxuries
- **Kenrick et al. 1993** — Rejection calibration
- **Henrich & Boyd 2001** — Cultural transmission, prestige bias
