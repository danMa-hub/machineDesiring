# MACCHINA DEL DESIDERIO — Open Questions

**versione 2.3 fixed_names — aggiornare ad ogni sessione**
**Convenzione: X.0 = maggiore, X.Y = minore (vedi project_instructions)**

---

## Verifiche risolte in v2.0

| ID | Descrizione | Risoluzione |
|:---|:---|:---|
| VERIFICA-A | SelfWorth coefficiente 60/40 | **Eliminata.** ConformityScore e SelfWorth rimossi. MySelfEsteem ora è derivata solo per BehaviorTree UE5. |
| VERIFICA-B | DesiredTarget liberi vs vincolati | **Risolta.** Formula assortativa standard: `myDesired_X = r_a×my + r_asp×ideal + r_pop×pop_mean + noise`. |
| VERIFICA-C | AcceptanceRange riprogettazione | **Eliminata.** Sostituito da σ della gaussiana (Conroy-Beam 2016). |
| VERIFICA-D | Archetipi validazione | **Eliminati.** Nessuna validità scientifica. |
| VERIFICA-G | Normalizzazione separata | **Mantenuta.** `myWeightsVisual`, `myWeightsSexual`, `myWeightsSingle` normalizzati separatamente. |
| VERIFICA-H | MySelfWorthBase vs MySelfWorth | **Eliminata.** Tutto il meccanismo sostituito da `MySelfEsteem` derivata. |
| VERIFICA-L | Aggregazione finale | **Risolta.** Distanza Euclidea pesata (Conroy-Beam). Soglia 0.35 fissa in v2.3. |
| VERIFICA-P | Bug DesireScore01 bipolari | **Risolta.** `(myDesired-trait)²` penalizza in entrambe le direzioni per design. |

---

## Verifiche risolte in v2.1–v2.3

| ID | Descrizione | Risoluzione |
|:---|:---|:---|
| TODO-5 | MyPenisSize indipendente, BT condizionato | **Risolto v2.1.** MyPenisSize in Passata 1 (ind), MyBottomTop condizionato a PenisSize×0.10. |
| TODO-7 | MyMuscle/MySlim piecewise age decay | **Risolto v2.1.** Picco 35yr, calo accelerato >60 (Doherty 2003). MySlim NHANES calibrato. |
| TODO-8 | MySexActDepth, MySubDom, MySelfEsteem+PenisSize | **Risolto v2.1.** SexActDepth base 0.38, SubDom BT×0.10, SelfEsteem +PenisSize×0.12. |
| TODO-9 | myTribeTag prototype scoring, myKinkTypes flag | **Risolto v2.1.** Distanza euclidea pesata vs prototipi, min_score=0.32. myKinkTypes come flag multipli (Parsons 2010). |
| TODO-10 | Appeal tripartito | **Risolto v2.2+v2.3.** `myPop_visAttraction` (tutti), `myPop_sexAttraction` (gated su `pair_visMatched`), `myPop_globalAttraction` = 0.60×vis + 0.40×sex. Bug fix v2.3: 0-mutual→globalAttraction=visAttraction. |
| TODO-11 | `myDesired_Hair` coefficienti | **Risolto v2.2.** r_assort 0.35→0.15 (Muscarella 2002), noise 0.18→0.28 (Barthová 2017). |
| TODO-12 | `myWeightsVisual` gay-specifici | **Risolto v2.2+v2.3.** Muscle 0.28, Beauty 0.23, Slim 0.15, Masc 0.15, Hair 0.05. |
| TODO-13 | `myWeightsSexual` correlati al profilo | **Risolto v2.2+v2.3.** BT extremity ×0.60, KinkLevel ×0.80, bottomness ×0.80. |
| BUG-1 | PozStatus af normalizzato via | **Risolto v2.3.** af modulava tutti e 3 gli elementi pw[] — normalizzazione annullava. Fix: af modula solo hiv_rate. |
| BUG-2 | `myPop_globalAttraction` media con 0 per 0-mutual | **Risolto v2.3.** Era (vis+0)/2. Fix: np.where — se 0 mutual → globalAttraction=visAttraction. |
| BUG-3 | MySelfEsteem drift | **Risolto v2.3.** Base 0.27→0.21 dopo aggiunta PenisSize×0.12. |
| BUG-4 | `pop_neverVisDesired` calcolata su uscenti invece che ricevute | **Risolto fixed_names.** `vis_pass.sum(axis=1)` misura attrazioni uscenti. Fix: aggiunto `in_vis = vis_pass.sum(axis=0)`. Ora distinto da `pop_neverVisMatched`. |
| BUG-5 | `prefs` corrotto in `premy_sexFrustration` nella Vergine | **Risolto fixed_names.** sed `fs→my_sexFrustration` aveva sostituito `prefs` perché `fs` è substring. Fix chirurgico. |
| FIX-5 | MyBeauty age decay troppo debole | **Risolto v2.3.** 0.08→0.20. Schope 2005, Rhodes 2006. |
| FIX-6 | MySafetyPractice: PrEP per HIV+ | **Risolto v2.3.** PrEP solo per negativi. Categorie ristrutturate per MyPozStatus. |
| FIX-7 | Noise `myWeightsVisual` uniforme | **Risolto v2.3.** MyMasculinity e MyHair 40% (polarizzazione), resto 30%. |
| FIX-8 | MyHair peso 0.08 segregava | **Risolto v2.3.** 0.08→0.05. |
| FIX-9 | MyExhibitionism nomenclatura | **Risolto v2.3.** Rinominato MyBodyDisplay. |
| FIX-10 | ExhibVoyeur/Breeding mancanti in myKinkTypes | **Risolto v2.3.** ExhibVoyeur + Breeding (BT-condizionato) aggiunti. |
| FIX-11 | generate_report crash in APP mode | **Risolto v2.3.** Accesso chiavi non presenti — fix con .get() fallback. |
| RENAME-1 | Nomenclatura variabili inconsistente | **Risolto fixed_names.** Tutte le variabili rinominate secondo le convenzioni di prefisso in `variables_v2_3_fixed_names.md`. Vedi tabella riepilogativa sotto. |

### Riepilogo rename RENAME-1

| Vecchio | Nuovo | Dove |
|:---|:---|:---|
| `AgeNorm` / `HeightNorm` | `myAgeNorm` / `myHeightNorm` | profilo |
| `TribeTag` / `KinkTypes` | `myTribeTag` / `myKinkTypes` | profilo derivate |
| `Desired_*` | `myDesired_*` | preferenze |
| `WeightsVisual/Sexual/Single` | `myWeightsVisual/Sexual/Single` | preferenze |
| `ObserverEthBias` / `ObserverPozBias` | `myEthBias` / `myPozBias` | preferenze |
| `mv_visual` / `mv_sexual` / `mv_total` | `myPop_visAttraction` / `myPop_sexAttraction` / `myPop_globalAttraction` | metriche |
| `out_vis` / `out_mutual` / `out_full` | `my_visAttractionOutCount` / `my_visMatchCount` / `my_fullMatchCount` | metriche |
| `thr_visual` / `thr_sexual` | `my_visThreshold` / `my_sexThreshold` | Vergine |
| `frustr_visual` / `frustr_sexual` | `my_visFrustration` / `my_sexFrustration` | Vergine |
| `mutual_visual` / `pair_match` / `group_match` | `my_visMatchCount` / `my_fullMatchCount` / `my_groupMatchCount` | Vergine |
| `vis[i][j]` / `sex[i][j]` / `comb[i][j]` | `selfToOther_visAttraction_cruise` / `selfToOther_sexAttraction_cruise` / `selfToOther_combinedAttraction_app` | matrici |
| `Gini MV` | `pop_visAttractionPolarization` | pop metriche |
| `0-mutual %` | `pop_neverVisMatched_cruise` | pop metriche |
| `0-full %` | `pop_neverSexMatched_cruise` | pop metriche |
| `mutual pairs %` | `pairs_visCompatibilityRate_cruise` | pair metriche |
| `BT kill %` | `pairs_bottomTopIncompatibilityShare` | pair metriche |
| `Kink kill %` | `pairs_kinkIncompatibilityShare` | pair metriche |

---

## Verifiche pendenti

### [TODO-1] Versatile penalizzato nel match BottomTop

**Problema:** Versatile (BT~0.50) genera `myDesired_BottomTop`~0.50. Distanza alta sia da Bottom puro (0.10) che da Top puro (0.90). `pop_neverSexMatched_cruise` ~51% per i versatili.

**Status:** Monitorato. Con N=700 il gap si è ridotto. Accettabile come realistico.

**Priorità:** bassa.

### [TODO-2] BT×corpo interaction

**Rimosso ma da monitorare.** Interazione "bottom preferisce top muscoloso" rimossa (basata su un unico studio, Moskowitz 2011 non conferma).

### [TODO-3] σ — calibrazione empirica

**Problema:** σ_cruising=0.13 e σ_app=0.15 sono parametri artistici. La fase sessuale è ~4.5× più selettiva della visiva per effetto della distanza BT.

**Da cercare:** dati su tassi di match reali da app gay per calibrare σ.

### [TODO-4] Soglia fissa 0.35 in v2.3

**Problema:** soglia fissa per tutti in v2.3. La Vergine implementa soglie adattive `my_visThreshold` / `my_sexThreshold` con floor `MySelfEsteem × 0.55`. La Vergine a equilibrio converge a soglia media ~0.317 — coerente con il 0.35 di v2.3.

**Priorità:** risolto dalla Vergine; traduzione in C++ BehaviorTree UE5 come prossimo step.

### [TODO-Vergine-1] my_visFrustration non differenziata

**Problema:** `my_visFrustration` satura al massimo per quasi tutti (mean=1.997, sd=0.028) indipendentemente da `myPop_visAttraction`. Chi è molto desiderato dovrebbe avere frustrazione visiva bassa.

**Causa:** il floor `MySelfEsteem × 0.55` è troppo basso per chi ha `myPop_visAttraction` alto — scende comunque al floor prima che i match rinforzino abbastanza la soglia.

**Priorità:** media — da affrontare prima della traduzione C++.

---

## Decisioni aperte

### [APERTA-3] Soglia fissa vs dinamica

In v2.3 soglia 0.35 fissa come baseline. La Vergine implementa la versione dinamica. Per UE5: BehaviorTree usa `my_visThreshold` / `my_sexThreshold` della Vergine come stato iniziale baked.

### [APERTA-6] Dual mode — σ diversi

σ_cruising=0.13 (7 vis + 4 sex variabili), σ_app=0.15 (11 insieme). I σ diversi compensano la dimensionalità. Documentato come scelta, non come bug.

### [APERTA-7] Nomenclatura file Python

- `macchina_v2_3_fixed_names.py` — simulazione statica baseline (ex v2.3)
- `macchina_vergine_1_0.py` — simulazione dinamica con soglie adattive

---

## Agenda prossima sessione

**Priorità alta:**
1. Traduzione algoritmo in C++ per UE5 — struttura dati NPC
2. BehaviorTree UE5 — `my_visThreshold` / `my_sexThreshold` / `my_visFrustration` / `my_sexFrustration`

**Priorità media:**
3. TODO-Vergine-1 — correggere `my_visFrustration` non differenziata
4. TODO-3 — cercare dati match rate da app gay per calibrare σ

**Priorità bassa:**
5. TODO-1 — monitorare Versatile con campioni più grandi
6. TODO-2 — monitorare letteratura BT×corpo
