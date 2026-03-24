# MACCHINA DEL DESIDERIO — Algoritmo v2.4

**Walkthrough discorsivo del codice Python (cambia solo con versioni maggiori)**

---

## 1. Panoramica

Il sistema genera N personaggi (default 200), ognuno con un `IndividualProfile` e un set di `MatingPreferences`. Poi calcola, per ogni coppia A→B, quanto A è attratto da B. L'attrazione è asimmetrica.

Due modalità:
- **Cruising** (two-step): prima visivo (`myOut_visAttraction_cruise`), poi sessuale (`myOut_sexAttraction_cruise`) — modella l'incontro fisico
- **App** (single-pass): unico score (`myOut_attraction_app`) — modella la dating app

Due file Python:
- `macchina_base_v2_4.py` — simulazione statica baseline (soglia 0.35 fissa)
- `macchina_vergine_*.py` — simulazione dinamica con soglie adattive

---

## 2. Generazione del profilo individuale

Tre passate. La prima genera le variabili indipendenti. La seconda le condizionate. La terza le derivate.

### Passata 1 — Indipendenti

```python
p['MyAge']       = clamp(np.random.normal(38.0, 15.7), 18, 100)
p['MyHeight']    = np.random.normal(177.0, 7.0)
p['MyPenisSize'] = clamp(np.random.normal(0.50, 0.15))  # biologico (Veale 2015)
```

L'età media è 38 — scelta artistica deliberata. `MyPenisSize` in Passata 1 perché biologico, non causato dal ruolo sessuale.

### Passata 2 — Condizionate

**MyMuscle** — piecewise con l'età: bonus <35yr, calo moderato 35-60, accelerato >60 (Doherty 2003). **MyBeauty** — decay forte (−0.20×ageNorm). **MyBottomTop** — trimodale B28%/V50%/T22%, condizionato a `myAgeNorm`, `myHeightNorm`, `MyPenisSize`. **MyPozStatus** — age-dependent, tasso base 18%, due terzi HIV+ sono PozUndet (CDC 2022).

### Passata 3 — Derivate

**MySelfEsteem** — sociometro: base 0.21 + Beauty×0.20 + Muscle×0.15 + Slim×0.10 + Masc×0.08 + PenisSize×0.12 − AgeNorm×0.10 + noise. Target ~0.48. **myTribeTag** — prototype scoring euclideo (SOLO LOD/debug). **myKinkTypes** — flag multipli condizionati a KinkLevel e SexExtremity (Parsons 2010).

---

## 3. Generazione delle preferenze

### La formula assortativa

```python
# Aspirazionale (r_ideal > 0)
myDesired_X = r_self × my_val + r_ideal × ideal + r_pop × pop_mean + noise

# Complementare (r_self < 0)
myDesired_X = |r_self| × (1 - my_val) + (1 - |r_self|) × pop_mean + noise

# Assortativo puro (r_ideal = 0)
myDesired_X = r_self × my_val + (1 - r_self) × pop_mean + noise
```

**Nota v2.4:** `r_self` (ex `r_assort`) = ancoraggio al proprio tratto. `r_ideal` (ex `r_aspir`) = pull verso ideale culturale. `r_pop` invariato.

### myWeightsVisual, myWeightsSexual — noise e modulazione

Noise: 30% per la maggior parte, 40% per `MyMasculinity` e `MyHair` (polarizzazione masc4masc/Bear). Modulazione: `bt_extremity` → peso BT ×(1+extremity×0.60), `MyKinkLevel` → peso KL ×(1+KL×0.80), `bottomness` → peso PS ×(1+bottomness×0.80).

### myWeightsSingle — normalizzazione unica (v2.4)

```python
# Pesi raw dalla letteratura → normalizzati a somma 1
# Nessuno split vis/sex artificiale — il rapporto emerge dai dati
_RAW = {Muscle:0.28, Beauty:0.23, ..., BottomTop:0.40, SubDom:0.28, ...}
BASE_WEIGHTS_SINGLE = {k: v / sum(_RAW.values()) for k, v in _RAW.items()}
# BottomTop: 0.40/1.97 = 0.203 — domina perché funzionalmente vincolante
```

Stesse modulazioni applicate. Il 62/38 precedente era una stima editoriale senza fonte empirica diretta su app gay.

---

## 4. Calcolo dell'attrazione — Modalità CRUISING

### Fase 1 — myOut_visAttraction_cruise

A valuta B su 7 variabili percepibili a vista. Distanza Euclidea pesata con `myWeightsVisual` di A. σ=0.13. Moltiplicatori post-distanza: `myEthBias` (etnia di B) e `myPozBias` (PozStatus di B).

`pair_visMatched = true` se `myOut_visAttraction_cruise[A→B] ≥ soglia` AND `myOut_visAttraction_cruise[B→A] ≥ soglia`.

### Fase 2 — myOut_sexAttraction_cruise

Solo se `pair_visMatched = true`. 4 variabili sessuali con `myWeightsSexual`. σ=0.13. Dealbreaker BT: distanza > 0.60 → ×0.10.

`pair_visSexMatched = true` se entrambi superano la soglia sessuale. ~88% delle `pair_visMatched` fallisce qui.

---

## 5. Calcolo dell'attrazione — Modalità APP

Unica distanza Euclidea su 11 variabili con `myWeightsSingle` (pesi normalizzati, BottomTop=0.203). σ=0.15. Dealbreaker BT: distanza > 0.60 → ×0.15 (meno severo perché info è preventiva).

**Comportamento BT in app:**
- Bottom (BT=0.10): desired≈0.82. Vs Top (BT=0.90): distanza 0.08 ✓ | Vs Bottom (BT=0.10): distanza 0.72 → dealbreaker ✓
- Versatile (BT=0.50): desired≈0.46. Distanza max vs chiunque = 0.44 → mai dealbreaker ✓

`pair_fullMatched = true` se entrambi superano la soglia. Nessuna frustrazione sessuale post-match per definizione.

---

## 6. Appeal emergente (myPop_)

### Cruising — tripartito

- **`myPop_visAttraction`**: `vis.mean(axis=0)` — media attrazioni visive ricevute da tutti.
- **`myPop_sexAttraction`**: media attrazioni sessuali ricevute, gated su `pair_visMatched`. Se nessun mutual visual → 0.
- **`myPop_globalAttraction`**: 0.60×vis + 0.40×sex. Se 0 mutual visual → uguale a visAttraction.

### App — unico

`myPop_globalAttraction = comb.mean(axis=0)`. Vis e sex non sono separati.

---

## 7. Nuove metriche v2.4

### my_visAttractionBalance

```python
my_visAttractionBalance = myIn_visAttractionCount / myOut_visAttractionCount
# >1 = desiderato più di quanto desidera (mercato favorevole)
# <1 = desidera più di quanto riceve (frustrazione strutturale)
# 0  = non ha trovato nessuno attraente (NPC selettivissimo o isolato)
```

Cattura l'asimmetria di mercato per-NPC. Complementa `myPop_visAttraction` (media continua) con un rapporto basato su soglia binaria.

### my_visToSexFrustrationRate

```python
my_visToSexFrustrationRate = (my_visMatchCount_cruise - my_visSexMatchCount_cruise) \
                              / my_visMatchCount_cruise
# 0.0 = tutti i vis match convertono a sex match (raro)
# 1.0 = nessun vis match converte (frustrante)
# NaN/0 per NPC senza vis match (impostato a 0.0)
```

Media ~0.88 in cruising: l'88% dei mutual visual non converte. Principale causa: incompatibilità BT (23%), kink (11%), distanza generale (66%).

---

## 8. Risultati tipici (N=700, soglia 0.35)

### Cruising

`pop_neverSexMatched_cruise` ~51%. `pop_neverVisMatched_cruise` ~14%. `pop_visAttractionPolarization` ~0.313. `r(MyBeauty, myPop_visAttraction)` +0.48. `r(MyMuscle, myPop_visAttraction)` +0.49.

### App (v2.4 nuovi pesi)

Con BottomTop a 0.203 (vs 0.152 v2.3): atteso lieve aumento esclusione same-role. `pop_neverMatched_app` da verificare con baseline. Gini atteso simile (~0.23).

### Entrambi

`r(MyAge, myPop_visAttraction)` −0.21. `r(MyPenisSize, myPop_sexAttraction)` +0.22. `MyMasculinity` e `MyHair` ~0 nella media (preferenze polarizzate si annullano). Black neverVisMatched ~14% vs White ~12%. Poz neverSexMatched ~29% vs None ~11%.

---

## 9. Guida alle metriche (v2.4)

### Gerarchia di inclusione garantita (cruising)

```
pop_neverVisDesired_cruise (4%)
  ⊆ pop_neverVisMatched_cruise (14%)
    ⊆ pop_neverSexMatched_cruise (51%)
```

### Predittori chiave

`myPop_visAttraction` è il predittore più forte di `my_visMatchCount_cruise` (r≈+0.72) e di `my_visSexMatchCount_cruise` (r≈+0.53). `myPop_globalAttraction` predice meglio `my_visSexMatchCount_cruise` (r≈+0.58) perché incorpora entrambe le dimensioni.

### my_visAttractionBalance — interpretazione

La media del balance è ~1.0 per costruzione matriciale (somma uscenti = somma entranti). La varianza è informativa: alta varianza → mercato polarizzato. Mediana << 1.0 indica distribuzione asimmetrica (pochi ricevono molto, molti ricevono poco).

### Vergine — distinzione Pop / Acc / Curr

`myPop_visAttraction` è il valore omnisciente calcolato sull'intera popolazione simultaneamente (solo v2.4 statico). In simulazione round-based: `myAcc_visAttraction_cruise` per il cumulato fino al round R, `myCurr_visAttraction_cruise` per lo snapshot del round corrente.
