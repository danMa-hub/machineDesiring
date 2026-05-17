# CLAUDE.md — Macchina del Desiderio
# Bussola di progetto — aggiornare a ogni sessione

---

## SETUP — esegui a ogni apertura

### 1. Struttura cartelle (esegui solo se mancano)

```bash
# Verifica e crea se non esistono
if not exist "D:\MachineDesiring\assets" (
  mkdir D:\MachineDesiring\assets\environments\ep01-beach\pineta
  mkdir D:\MachineDesiring\assets\environments\ep01-beach\scogliere
  mkdir D:\MachineDesiring\assets\environments\ep01-beach\zona-cruising
  mkdir D:\MachineDesiring\assets\environments\ep01-beach\area-04
  mkdir D:\MachineDesiring\assets\environments\ep01-beach\area-05
  mkdir D:\MachineDesiring\assets\environments\ep01-beach\area-06
  mkdir D:\MachineDesiring\assets\environments\ep01-beach\area-07
  mkdir D:\MachineDesiring\assets\characters
  mkdir D:\MachineDesiring\assets\props
  mkdir D:\MachineDesiring\simulation
)

# Sposta .blend se ancora alla root
if exist "D:\MachineDesiring\testSetCruising.blend" (
  move D:\MachineDesiring\testSetCruising.blend D:\MachineDesiring\assets\environments\ep01-beach\
  move D:\MachineDesiring\testSetCruising.blend1 D:\MachineDesiring\assets\environments\ep01-beach\
)

# Elimina shortcut inutile
if exist "D:\MachineDesiring\assetsxBlender - collegamento" (
  del "D:\MachineDesiring\assetsxBlender - collegamento"
)
```

### 2. Verifica file simulazione (esegui sempre)

Controlla se i seguenti file esistono in locale. Se mancano, segnalalo.

**File Python — devono stare in `D:\MachineDesiring\simulation\`:**
```
macchina_base_v2_4.py
macchina_vergine_totale_1_1.py
```

**File documentazione — devono stare in `D:\MachineDesiring\simulation\docs\`:**
```
foundation_macchina_base_v2_4.md
foundation_vergine_totale.md
variables_macchina_base_v2_4.md
variables_vergine_addendum_1_1.md
algorithm_walkthrough_macchina_base_v2_4.md
algorithm_walkthrough_vergine_1_1.md
open_questions_v2_3_fixed_names.md
open_questions_vergine_totale.md
bibliography_v2_3.md
```

Se li trovi in `MachineDesiringUnreal\Documentation_MachineDesiring_base_v-2-4\`
o altrove, spostali nelle cartelle corrette sopra e segnala cosa hai spostato.

---

## FOCUS CORRENTE

```
Episodio  : 01 — Spiaggia Croatia
Livello   : ep01_beach (da creare)
Area attiva: —
Fase      : pianificazione struttura → [concept micro-aree] → asset → Blueprint
Prossimo  : definire le 7-8 micro-aree di ep01-beach (.claude/ep01-beach.md)
            poi implementare BP_Execute* in BP_NPC_AIController (UE5)
```

---

## Progetto

Installazione artistica interattiva in UE5.7. 100 NPC autonomi simulano dinamiche
relazionali e sessuali della comunità gay maschile. Omaggio alla Macchina Celibe di
Duchamp — sistema che produce desiderio senza risolverlo.

**Temi:** desiderio come motore irrisolvibile · stigma e gerarchia estetica nella
cultura gay · solitudine strutturale · dinamiche di cruising e chemsex · il corpo
come capitale sociale.

L'utente sta imparando C++ in UE5. Spiega ogni passo chiaramente.
Implementa un passo alla volta. Compila prima di procedere.

**Repository locale:**  `D:\MachineDesiring\` (root git)
**Repository GitHub:**  `https://github.com/danMa-hub/machineDesiring` (branch: main)
**Progetto Unreal:**    `D:\MachineDesiring\MachineDesiringUnreal\`

---

## Struttura repository

```
D:\MachineDesiring\                  ← root git
  CLAUDE.md                          ← questo file (bussola)
  .claude/                           ← contesto esteso per Claude Code
    simulation.md                    ← 3 modelli Python, formule, metriche
    variables.md                     ← variabili, pesi, pipeline
    roadmap.md                       ← TODO unificati Python + UE5
    literature.md                    ← fonti con risultato chiave
    ep01-beach.md                    ← visione livello, micro-aree, stato asset
  
  assets/                            ← tutto il lavoro esterno a UE5
    environments/
      ep01-beach/
        pineta/                      ← .blend + .hip + .fbx + README.md
        scogliere/
        zona-cruising/
        area-04..07/
    characters/
    props/
  
  simulation/                        ← file Python
  assetsxBlender/                    ← export UE5→esterno (non toccare, genera UE5)
  MachineDesiringUnreal/             ← progetto UE5 (non toccare la struttura)
```

**Ogni cartella asset contiene tutto il ciclo di lavorazione:**
```
pineta/
  pineta.blend        ← modellazione/shading (Blender)
  pineta.hip          ← procedural/simulazione (Houdini)
  pineta_export.fbx   ← export → UE5
  pineta_ref.abc      ← Alembic se simulazioni muscoli/cloth
  README.md           ← concept, stato, todo specifico dell'area
```

---

## Principi metodologici — NON MODIFICARE SENZA DISCUSSIONE

1. Distribuzioni demografiche reali (non delle app) — età media 38, muscle media 0.30
2. Bias sistemici come realtà documentata — non attenuare etnia/poz per correttezza
3. Match cruising in due fasi temporali distinte (visual → sexual)
4. myTribeTag solo per LOD e debug — mai per match individuale
5. myDesired_ da formula assortativa — non liberi, non random
6. Normalizzazione pesi separata per step (visual / sexual / single-pass)
7. myEthBias / myPozBias generati una volta per persona — non per coppia
8. Pesi app: normalizzazione unica senza split artificiale vis/sex
9. Tutti i pesi da letteratura gay-specifica — non da campioni eterosessuali

---

## Modelli simulazione — overview

Dettaglio completo in @.claude/simulation.md

```
v2.4 (statico)          macchina_base_v2_4.py
  Fotografia istantanea. N=700, soglia fissa 0.35, matrice N×N.
  Baseline di calibrazione. Risultati: 14% neverVisMatched,
  51% neverSexMatched, Gini 0.313.

Vergine v1.0 (soglie adattive)    macchina_vergine_1_0.py (ref)
  NPC impara quanto vale — target fissi, soglie adattive.
  Convergenza a soglia μ=0.318. Modello di produzione per UE5.

Vergine Totale v1.1 (desiderio appreso)   macchina_vergine_totale_1_1.py
  NPC parte senza preferenze — le forma dagli incontri.
  Modello esplorativo/narrativo.
```

Dual mode: MODE_CRUISING (two-step: visual→sexual) / MODE_APP (single-pass 11 var).
Formula: distanza euclidea pesata → exp(-d²/2σ²). Fonte: Conroy-Beam 2016-2022.

---

## Build UE5

```
VS Code:  Ctrl+Shift+B → "Build MachineDesiringEditor (Development)"  [default]
Claude:   "C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/Build.bat"
          MachineDesiringEditor Win64 Development -Project="...uproject" -WaitMutex
Editor:   Tools > Compile (solo modifiche minori, hot reload)
```
Dopo C++: chiudi editor UE5 completamente → build → riapri.

**Plugin richiesti:**
- `Mover` (experimental) — movimento GASP/Mover 2.0
- `Motion Matching` — locomotion system
- `Chooser Framework` (experimental) — selezione animazioni GASP
- `Animation Warping` (experimental)
- `Motion Warping`

---

## Struttura Source C++

```
Source/MachineDesiring/
  Population/
    F_npcProfile.h/.cpp           ← profilo NPC + enum + pesi
    U_populationSubsystem.h/.cpp  ← 700 profili + matrici attrazione N×N

  NPC/
    NpcMemoryTypes.h              ← enum e struct condivisi (non modificare)
    A_navMeshZone.h/.cpp          ← zone Idle/Mating sul NavMesh
    U_npcBrainComponent.h/.cpp    ← TUTTI i dati runtime NPC
                                     Aggiungibile a qualsiasi APawn
    A_npcCharacter.h/.cpp         ← shell minima ACharacter (NON usata da GASP)
    A_npcController.h/.cpp        ← logica: percezione, segnalazione, soglie, mating
    A_playerController.h/.cpp     ← toggle debug (1=draw, 2=ID, 3=PopPanel, 4=Runtime)
    A_spawnManager.h/.cpp         ← spawn APawn generici

  Overlay/
    U_overlaySubsystem.h/.cpp     ← aggregatore dati UI
    UW_mainOverlay.h/.cpp         ← widget principale
    UW_populationPanel.h/.cpp     ← VIEW 1: statistiche popolazione
    UW_runtimePanel.h/.cpp        ← VIEW 2: runtime panel (WIP)
```

**Build.cs:** `Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, NavigationSystem`
+ PrivateDependency: `UMG, Slate, SlateCore`
`PublicIncludePaths` copre tutte le sottocartelle — usa include flat nei .cpp.

---

## Naming convention — OBBLIGATORIA

### Prefissi tipo UE5
| Prefisso | Tipo                              | Esempio                    |
|----------|-----------------------------------|----------------------------|
| `A_`     | Actor                             | `A_npcCharacter`           |
| `U_`     | UObject / Subsystem / Component   | `U_npcBrainComponent`      |
| `F_`     | Struct (USTRUCT/GENERATED_BODY)   | `F_npcProfile`             |
| `E_`     | Enum                              | `E_myState`                |

### Variabili runtime NPC
| Prefisso       | Scope                        | Esempio               |
|----------------|------------------------------|-----------------------|
| `My` PascalCase | tratto profilo (F_npcProfile) | `MyMuscle`, `MyAge`  |
| `my` camelCase  | variabile runtime per-NPC    | `myCurrentState`      |
| `myOut_`        | score/segnale uscente        | `myOut_signalValue`   |
| `myIn_`         | score/segnale entrante       | `myIn_signalValue`    |
| `b_my`          | bool per-NPC                 | `b_myIsSignaling`     |
| `myTimer_`      | timer handle                 | `myTimer_SignalingLogic` |
| `wVis_`/`wSex_`/`wSin_` | pesi normalizzati flat | `wVis_Muscle`    |

**Nome file = nome classe esatto:** `A_npcCharacter.h/.cpp`

---

## Enums (NpcMemoryTypes.h)

```cpp
E_myState          { Cruising, Mating, Latency }
E_cruisingSubState { RandomWalk, IdleWait, MatingWait, TargetWalk }
E_otherIDState     { Desired, NotYetDesired, GaveUp, Lost, SexBlocked }
E_navMeshZoneType  { Idle, Mating }
E_zoneShape        { Sphere, Box }
```

**F_otherIDMemory:**
```cpp
int32           targetId          // -1 = non inizializzato
E_otherIDState  state
float           myOut_signalValue // segnali inviati, accumulati
int32           myOut_signalCount
float           myIn_signalValue  // sempre == RecPartner->myOut (sincronizzato)
int32           myIn_signalCount
float           myTLastSeen       // GetWorld()->GetTimeSeconds()
```
`myOut_visAttraction` / `myOut_sexAttraction` NON qui — leggere via `GetVisScore()` / `GetSexScore()` O(1).
**TMap pointer instability:** non usare puntatori a campi interni tra TMap di NPC diversi.

---

## Architettura GASP/Mover

`SandboxCharacter_Mover` è Blueprint — impossibile ereditare in C++.
**Soluzione:** `U_npcBrainComponent` (UActorComponent) aggiungibile a qualsiasi APawn.
`BP_NPC_Character` eredita `MY_MOVERTEST` e ha il BrainComponent nel Blueprint Editor.

### Split C++ / Blueprint
| C++ (A_npcController)              | Blueprint (BP_NPC_AIController)       |
|------------------------------------|---------------------------------------|
| Calcola destinazione NavMesh       | Esegue movimento fisico (CharacterMover) |
| Decide sub-stato, timing fasi      | Gestisce animazioni (Motion Matching) |
| Segnalazione, soglie, mating trigger | Implementa BP_Execute* events       |
| Percezione entry point             | Configura AIPerception                |

### Interfaccia C++ → Blueprint (BlueprintImplementableEvent)
```cpp
BP_ExecuteRandomWalk(FVector Destination)
BP_ExecuteIdleWait(FVector Location, float Duration, float BaseYaw, float PhaseOffset)
BP_ExecuteMatingWait(FVector Location, float Duration, float BaseYaw, float PhaseOffset)
BP_ExecuteTargetWalk(FVector Destination, float SpeedMult, int32 TargetId)
BP_ExecuteMatingMove(FVector Destination)
BP_OnCruisingSubStateChanged(E_cruisingSubState NewSubState)
```

### Interfaccia Blueprint → C++ (BlueprintCallable)
```cpp
OnBPPhaseDone()       // fine fase → ChooseNext
OnBPMatingArrived()   // arrivato punto mating → placeholder U_matingLogic
OnBPMatingFailed()    // path fallito → retry dopo 2s
```

### Parametri Mutable (U_npcBrainComponent → Blueprint)
```cpp
MutableMuscle, MutableSlim, MutableHeight, MutableAge,
MutableBodyDisplay, MutableHair, MutableEthnicity
BP_ApplyMutableProfile()  // chiamato da SpawnManager dopo spawn
```

---

## Stato implementazione UE5

### U_populationSubsystem ✓
- 700 `F_npcProfile` generati con seed deterministico (Box-Muller, FRandomStream)
- `VisAttractionMatrix` / `SexAttractionMatrix` / `AppAttractionMatrix` N×N pre-calcolate
- `GetVisScore(ObsId, TgtId)` / `GetSexScore(ObsId, TgtId)` — O(1)
- `GetMeshScaleZ(NpcId)` — scala Z normalizzata su appeal visivo
- `CalculateSubsetStats(SpawnedIDs)` — metriche sul subset spawnato

### U_npcBrainComponent ✓
```cpp
int32                             myId
E_myState                         myCurrentState
TMap<int32, F_otherIDMemory>      mySocialMemory
TArray<int32>                     myDesiredRank          // max 20, VisScore desc
TSet<int32>                       myStaticVisBlacklist
bool                              b_myIsSignaling
TWeakObjectPtr<U_npcBrainComponent> mySignalTarget
float  myVisCurrThreshold / myVisMinThreshold / myVisStartThreshold
float  mySexCurrThreshold / mySexMinThreshold / mySexStartThreshold
float  myMaxWalkSpeed        // [70,100] cm/s deterministico (seed = myId+10000)
```

### A_npcController ✓ — costanti chiave
```cpp
SIGNAL_VALUES[]         = { 1.0, 0.8, 0.6, 0.4, 0.2 }
GAVEUP_SENT_THRESHOLD   = 5.0f
MATING_SIGNAL_THRESHOLD = 5.0f
LOST_TIMEOUT_SECONDS    = 600.0f
SIGNALING_TIMER_INTERVAL= 2.5f
WALK_RADIUS_MIN/MAX     = 500 / 2000 cm
TARGETWALK_REACH_DIST   = 150 cm
TARGETWALK_SPEED_MULT   = 1.8f
IDLE_DURATION_MIN/MAX   = 10 / 55 s
```

---

## Algoritmo Cruising — logica di riferimento

**FSM:** Cruising → Mating → Latency (da implementare)

**Movimento (E_cruisingSubState):**
```
id%20==0 → MatingWait (5%) | id%2==0 → IdleWait (~45%) | else → RandomWalk (~50%)
RandomWalk → IdleWait → MatingWait → TargetWalk (top-5 desiderati, vel 1.8×)
ChooseAndStartNextCruisingSubState: 3-way random a fine fase
```

**Segnalazione (ogni 2.5s):**
```
scan myDesiredRank[0..4] → primo valido in range ≤400cm + FOV 240°
→ TrySignaling → lock atomico → ExchangeSignals → ReleaseSignalLock dopo 1.5s
GaveUp:  out ≥ 5 && in < 1
Mating:  out ≥ 5 && in ≥ 5 su entrambi → SetState(Mating) → StartMatingMove
```

**Soglie adattive:**
`myTimeWithoutDesired ≥ myThresholdDropDelay` → drop soglia + promozione retroattiva
`NotYetDesired → Desired`

**SexBlocked:** permanente. Da implementare in U_matingLogic.
**Lost:** non visto >600s → Lost. Al re-avvistamento: rivalutazione vs soglia corrente.

---

## Content Browser UE5

```
Content/NPC/
  BP_NPC_Character      ← eredita MY_MOVERTEST, ha U_npcBrainComponent
  BP_NPC_AIController   ← eredita A_npcController, AIPerception sight 2000cm 60°

Content/Blueprints/
  SandboxCharacter_Mover  ← GASP base (non modificare)
  MY_MOVERTEST            ← subclass locomotion GASP

Content/Zones/
  A_navMeshZone actors  ← ZoneType: Idle/Mating, ZoneShape: Sphere/Box
```

---

## Prossimo step UE5

Vedi roadmap completa in @.claude/roadmap.md

**Priorità alta:**
- Implementare `BP_Execute*` in `BP_NPC_AIController` — movimento reale CharacterMover/NavMesh
- `U_matingLogic` — logica post-arrivo zona Mating, Branch A/B, SexBlocked
- Stato `Latency` — cooldown post-mating

**Priorità media:**
- `UW_runtimePanel` VIEW 2 — completare pannello runtime
- Plugin Mutable → `BP_ApplyMutableProfile`

---

## Regole C++ UE5 — critiche

1. **No `TMap`/`TArray` con `{}` initializer lists** come costanti statiche → C-style array
2. **No path subfolder negli include** → `#include "F_npcProfile.h"` non `#include "Population/..."`
3. **`generated.h` sempre ultimo** include in ogni header
4. **`UPROPERTY()`** su ogni `TObjectPtr` e `TMap<K, UObject*>` per GC
   - `TMap<int32, F_struct>` senza UPROPERTY è corretto (struct plain)
   - `TMap<int32, UObject*>` con UPROPERTY è obbligatorio
5. **`GENERATED_BODY()`** in ogni `USTRUCT` / `UCLASS`
6. **RNG deterministico:** `FRandomStream` per profili, `RNG.FRandRange` per runtime
7. **`TWeakObjectPtr`** per riferimenti cross-NPC
8. **TMap pointer instability:** non salvare puntatori a campi di struct in TMap
9. **`GetGameInstance()` null-check:**
   ```cpp
   if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
       MySubsystem = GI->GetSubsystem<U_mySubsystem>();
   ```
10. **Lambda timer:** `FTimerDelegate::CreateWeakLambda(this, [...](){...})` — mai lambda raw
11. **`IsValid(Ptr)`** per UObject — non serve Cast se il tipo è già UObject