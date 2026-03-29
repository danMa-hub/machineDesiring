# CLAUDE.md
# Macchina del Desiderio — UE5 C++ Reference

---

## Progetto

Installazione artistica interattiva in UE5.7. 100 NPC autonomi simulano dinamiche relazionali e sessuali della comunità gay maschile. Omaggio alla Macchina Celibe di Duchamp.

L'utente sta imparando C++ in UE5. Spiega ogni passo chiaramente. Implementa un passo alla volta. Compila prima di procedere.

**Repository:** `D:/THEGUTS/theGutsUnreal/MachineDesiring/MachineDesiring/`

**IDE:** Visual Studio Code + MSVC BuildTools 2022

**Build:**
```
VS Code:    Ctrl+Shift+B  →  "Build MachineDesiringEditor (Development)"  [default]
Claude:     "C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/Build.bat"
            MachineDesiringEditor Win64 Development -Project="...uproject" -WaitMutex
Editor:     Tools > Compile (solo modifiche minori, hot reload)
```
Dopo C++: chiudi editor UE5 completamente → build → riapri. Hot reload funziona solo per modifiche minori.

**Plugin richiesti (abilitare in Edit → Plugins):**
- `Mover` (experimental) — movimento GASP/Mover 2.0
- `Motion Matching` — locomotion system
- `Chooser Framework` (experimental) — selezione animazioni GASP
- `Animation Warping` (experimental)
- `Motion Warping`

---

## Struttura file

```
Source/MachineDesiring/
  Population/
    F_npcProfile.h/.cpp          ← profilo NPC + enum + pesi
    U_populationSubsystem.h/.cpp ← 700 profili + matrici attrazione N×N

  NPC/
    NpcMemoryTypes.h             ← enum e struct condivisi (non modificare)
    A_navMeshZone.h/.cpp         ← zone Idle/Mating sul NavMesh
    U_npcBrainComponent.h/.cpp   ← TUTTI i dati runtime NPC (myId, FSM, soglie, memoria)
                                    Aggiungibile a qualsiasi APawn — GASP, ThirdPerson, MetaHuman
    A_npcCharacter.h/.cpp        ← shell minima ACharacter (parent alternativo futuro, NON usata da GASP)
    A_npcController.h/.cpp       ← logica: percezione, segnalazione, soglie, mating trigger
                                    Movimento delegato al Blueprint via BlueprintImplementableEvent
    A_playerController.h/.cpp    ← toggle debug (tasto 1=draw, 2=ID, 3=PopPanel, 4=RuntimePanel)
    A_spawnManager.h/.cpp        ← spawn APawn generici, NpcById TMap<int32, U_npcBrainComponent*>

  Overlay/
    U_overlaySubsystem.h/.cpp    ← aggregatore dati UI, timer refresh
    UW_mainOverlay.h/.cpp        ← widget principale, switch viste
    UW_populationPanel.h/.cpp    ← VIEW 1: statistiche popolazione subset
    UW_runtimePanel.h/.cpp       ← VIEW 2: runtime panel (WIP)
```

**Build.cs dipendenze:**
```
Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, NavigationSystem
UMG, Slate, SlateCore  (PrivateDependency)
```
`PublicIncludePaths` copre tutte le sottocartelle — usa include flat nei .cpp.

---

## Naming convention — OBBLIGATORIA

### Prefissi tipo UE5
| Prefisso | Tipo | Esempio |
|---|---|---|
| `A_` | Actor | `A_npcCharacter`, `A_spawnManager` |
| `U_` | UObject/Subsystem/Component | `U_populationSubsystem`, `U_npcBrainComponent` |
| `F_` | Struct UE5 (con USTRUCT/GENERATED_BODY) | `F_npcProfile`, `F_otherIDMemory` |
| `E_` | Enum | `E_myState`, `E_npcEthnicity` |

### Variabili runtime NPC
| Prefisso | Scope | Esempio |
|---|---|---|
| `My` PascalCase | tratto profilo (`F_npcProfile`) | `MyMuscle`, `MyAge` |
| `my` camelCase | variabile runtime per-NPC | `myCurrentState`, `mySocialMemory` |
| `myOut_` | score/segnale uscente | `myOut_signalValue` |
| `myIn_` | score/segnale entrante | `myIn_signalValue` |
| `b_my` | bool per-NPC | `b_myIsSignaling` |
| `myTimer_` | timer handle | `myTimer_SignalingLogic` |
| `wVis_`/`wSex_`/`wSin_` | pesi normalizzati flat | `wVis_Muscle` |

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

**F_otherIDMemory** (in NpcMemoryTypes.h):
```cpp
int32           targetId             // -1 = non inizializzato
E_otherIDState  state
float           myOut_signalValue    // segnali inviati, accumulati
int32           myOut_signalCount
float           myIn_signalValue     // segnali ricevuti — sempre == RecPartner->myOut (sincronizzato)
int32           myIn_signalCount
float           myTLastSeen          // GetWorld()->GetTimeSeconds()
```
`myOut_visAttraction` / `myOut_sexAttraction` NON sono qui — leggere via `GetVisScore()` / `GetSexScore()` O(1).

**Nota TMap pointer stability:** `TMap<int32, F_otherIDMemory>` non ha indirizzi stabili dopo rehash. Non usare puntatori a campi interni tra TMap di NPC diversi.

---

## Architettura GASP/Mover (refactoring 2026-03-29)

### Problema chiave
`SandboxCharacter_Mover` è un Blueprint (.uasset) — impossibile ereditare in C++.
`MY_MOVERTEST` è una subclass Blueprint di `SandboxCharacter_Mover`.

### Soluzione: U_npcBrainComponent
Tutti i dati NPC sono su `U_npcBrainComponent` (UActorComponent), aggiungibile a qualsiasi APawn.
`BP_NPC_Character` eredita `MY_MOVERTEST` e ha il BrainComponent aggiunto nel Blueprint Editor.

### Split C++ / Blueprint
| C++ (A_npcController) | Blueprint (BP_NPC_AIController) |
|---|---|
| Calcola destinazione NavMesh | Esegue movimento fisico (CharacterMover) |
| Decide sub-stato, timing fasi | Gestisce animazioni (Motion Matching) |
| Segnalazione, soglie, mating trigger | Implementa BP_Execute* events |
| Percezione entry point | Configura AIPerception, chiama OnNearbyNPCsUpdated |

### Interfaccia C++ → Blueprint (BlueprintImplementableEvent su A_npcController)
```cpp
BP_ExecuteRandomWalk(FVector Destination)
BP_ExecuteIdleWait(FVector Location, float Duration, float BaseYaw, float PhaseOffset)
BP_ExecuteMatingWait(FVector Location, float Duration, float BaseYaw, float PhaseOffset)
BP_ExecuteTargetWalk(FVector Destination, float SpeedMult, int32 TargetId)
BP_ExecuteMatingMove(FVector Destination)
BP_OnCruisingSubStateChanged(E_cruisingSubState NewSubState)
```

### Interfaccia Blueprint → C++ (BlueprintCallable su A_npcController)
```cpp
OnBPPhaseDone()        // fine fase RandomWalk/IdleWait/TargetWalk/MatingWait → ChooseNext
OnBPMatingArrived()    // arrivato al punto mating → placeholder U_matingLogic
OnBPMatingFailed()     // path fallito → retry dopo 2s
```

### Parametri Mutable (su U_npcBrainComponent, letti dal Blueprint)
```cpp
MutableMuscle, MutableSlim, MutableHeight, MutableAge,
MutableBodyDisplay, MutableHair, MutableEthnicity
// Popolati da InitWithId() → ExtractMutableParams(F_npcProfile)
// Blueprint li legge per configurare UCustomizableSkeletalComponent (quando Mutable installato)
BP_ApplyMutableProfile()  // BlueprintImplementableEvent — chiamato da SpawnManager dopo spawn
```

---

## Stato implementazione

### U_populationSubsystem
- 700 `F_npcProfile` generati con seed deterministico (Box-Muller, FRandomStream)
- `VisAttractionMatrix` / `SexAttractionMatrix` / `AppAttractionMatrix` N×N pre-calcolate
- `GetVisScore(ObsId, TgtId)` / `GetSexScore(ObsId, TgtId)` — O(1)
- `GetMeshScaleZ(NpcId)` — scala Z [0.5, 4.0] normalizzata su appeal visivo
- `CalculateSubsetStats(SpawnedIDs)` — metriche sul subset spawnato

### U_npcBrainComponent (dati runtime NPC)
```cpp
int32                              myId
E_myState                          myCurrentState      // Cruising / Mating / Latency
TMap<int32, F_otherIDMemory>       mySocialMemory      // no UPROPERTY (struct plain)
TArray<int32>                      myDesiredRank       // max 20, ordinato per VisScore desc
TSet<int32>                        myStaticVisBlacklist
bool                               b_myIsSignaling
TWeakObjectPtr<U_npcBrainComponent> mySignalTarget
float  myVisCurrThreshold / myVisMinThreshold / myVisStartThreshold
float  mySexCurrThreshold / mySexMinThreshold / mySexStartThreshold
float  myMaxWalkSpeed       // [70, 100] cm/s, deterministico per-NPC (seed = myId+10000)
float  myRotationRate       // 60°/s default
// Materiali per sub-stato debug (assegnabili in Blueprint):
UMaterialInterface* Mat_RandomWalk / Mat_Idle / Mat_Mating / Mat_TargetWalk
```
- `myVisMinThreshold`: gauss(0.35-drop, 0.04) clamp, seed = myId+500
- `SetSubStateMaterial`: itera tutti gli `USkeletalMeshComponent` dell'owner

### A_npcController

**Costanti chiave (in .cpp):**
```cpp
// Segnalazione
SIGNAL_ACTIVE_DURATION   = 1.5f
SIGNAL_VALUES[]          = { 1.0, 0.8, 0.6, 0.4, 0.2 }   // rank 0..4
SIGNAL_VALUE_FALLBACK    = 0.1f
GAVEUP_SENT_THRESHOLD    = 5.0f
GAVEUP_RECV_THRESHOLD    = 1.0f
MATING_SIGNAL_THRESHOLD  = 5.0f
LOST_TIMEOUT_SECONDS     = 600.0f
SIGNALING_TIMER_INTERVAL = 2.5f

// RandomWalk
WALK_RADIUS_MIN/MAX      = 500 / 2000 cm
// Durata gestita dal Blueprint → OnBPPhaseDone() [riferimento: 30-90s]

// TargetWalk
TARGETWALK_RADIUS        = 150 cm
TARGETWALK_FORWARD_OFFSET= 300 cm   // offset avanti lungo il forward del TARGET
TARGETWALK_REACH_DIST    = 150 cm
TARGETWALK_SPEED_MULT    = 1.8f
// Durata gestita dal Blueprint → OnBPPhaseDone() [riferimento: 50-120s]

// Idle / MatingWait (via StartZoneWait)
IDLE_DURATION_MIN/MAX    = 10 / 55 s
IDLE_YAW_OFFSET          = 40°
```

**Timer attivi per NPC:**
```
myTimer_InitialMovement  — 0.2s one-shot — startup ritardato dopo OnPossess
myTimer_SignalingLogic   — 2.5s loop — scan handshake
myTimer_LostCheck        — 300s loop — scansione Lost
myTimer_TargetWalkTick   — 2s loop — aggiorna destinazione TargetWalk
myTimer_MatingRetry      — 2s one-shot — retry mating se path fallisce
myTimer_DebugDraw        — 0.2s loop — label debug (early return se debug off)
```

**Percezione (Blueprint-side):**
- AIPerception con sight cone (2000cm, 60°) su `BP_NPC_AIController`
- `OnTargetPerceptionUpdated` (per-actor):
  - bCurrentlySensed=true → `OnNearbyNPCsUpdated([id])`
  - bCurrentlySensed=false → `OnNpcLostFromSight(id)`
- `ProcessPerceivedNpc`: blacklist O(1) → TMap.Find O(1) → VisScore → rank

**Movimento (E_cruisingSubState):**
```
StartInitialMovement (0.2s dopo OnPossess):
  id%20==0 → MatingWait (5%)
  id%2==0  → IdleWait  (~45%)
  else     → RandomWalk (~50%)

RandomWalk:   anello [500,2000]cm → BP_ExecuteRandomWalk(Destination)
IdleWait:     zona Idle più vicina → BP_ExecuteIdleWait(Location, Duration, BaseYaw, PhaseOffset)
MatingWait:   zona Mating più vicina → BP_ExecuteMatingWait(...) [stesso helper StartZoneWait]
TargetWalk:   insegue top-5 desiderati (solo Cruising), vel 1.8×, TickTargetWalk 2s loop
              → BP_ExecuteTargetWalk(Destination, SpeedMult, TargetId)

ChooseAndStartNextCruisingSubState: 3-way random al termine di ogni fase (OnBPPhaseDone)
StartZoneWait(ZoneType, SubState): helper comune per IdleWait e MatingWait
```

**Segnalazione (TickSignalingLogic ogni 2.5s):**
```
guard b_myIsSignaling → return
guard state != Cruising → return
loop myDesiredRank[0..4]:
  skip stati terminali (GaveUp, Lost, SexBlocked)
  check disponibilità (bool)
  check distanza ≤ 400cm
  check FOV: dot(ForwardA, DirAtoB) > -0.2 su entrambi  ← cono ~240° per NPC
  → TrySignaling → break
accumulo myTimeWithoutDesired → DropVisThresholdAndPromote se ≥ myThresholdDropDelay
```

**TrySignaling:** lock atomico b_myIsSignaling su entrambi → ExchangeSignals → ReleaseSignalLock dopo 1.5s.

**ExchangeSignals:**
1. Calcola SentByA (rank di B in A) e SentByB (rank di A in B, 0 se assente)
2. Incrementa `myOut` solo se stato non terminale
3. **Sincronizzazione forzata:** `RecA->myIn = RecB->myOut` e viceversa
4. Debug line: verde se reciproco, rosso se unilaterale (solo se bDebugDrawEnabled)
5. `OnSignalExchanged(A)` → ri-cerca RecA (potrebbe essere cambiato) → se A non GaveUp → `OnSignalExchanged(B)`

**OnSignalExchanged:**
- Guard: `myCurrentState == Mating` → return (previene double trigger)
- Guard: stato record terminale → return
- GaveUp: out ≥ 5 && in < 1 → GaveUp, rimosso da rank
- Mating trigger: out ≥ 5 && in ≥ 5 su entrambi i lati → SetState(Mating) su entrambi
  → `NotifyMatingStarted` → `StartMatingMove` (stesso punto per entrambi)

**ReleaseSignalLock:** non resetta b_myIsSignaling su NPC in stato Mating.

**Mating:**
- `NotifyMatingStarted(IdA, IdB)`: notifica overlay + marca GaveUp in tutti gli NPC che avevano IdA/IdB nel rank
- `StartMatingMove(Point)`: clears TUTTI i timer cruising → `BP_ExecuteMatingMove(Point)`
  - path fallito → `OnBPMatingFailed` → retry dopo 2s (CreateWeakLambda)
- `StartMatingMove` e `OnSignalExchanged` sono **public** (chiamate cross-controller)

**Memoria:**
- `TickLostCheck` ogni 300s: Lost se `Now - myTLastSeen > 600s`
- `DropVisThresholdAndPromote`: drop soglia + promozione retroattiva NotYetDesired → Desired
- `SortDesiredRank`: sort per VisScore desc, chiamato dopo ogni Add

**Debug:**
- `bDebugDrawEnabled` (tasto 1): label sub-stato colorato sopra NPC + linee segnalazione
- `bDebugIdEnabled` (tasto 2): ID overlay, mostra "ID → targetID" in TargetWalk
- Timer DebugDraw: 0.2s loop → `TickDebugDraw` (early return se entrambi i flag off)
- Getter `GetCruisingSubState()` esposto per overlay inspector

**Parametri editabili:**
```cpp
myThresholdDropDelay    = 60f    // EditAnywhere
myVisThresholdDropStep  = 0.05f  // EditAnywhere
```

---

## Algoritmo Cruising — logica di riferimento

### FSM principale
- **Cruising** — stato default. Segnalazione attiva. Movimento su NavMesh.
- **Mating** — accoppiamento. Locomozione verso zona Mating concordata.
- **Latency** — cooldown post-mating (da implementare).

### Segnalazione rank-based
Ogni 2.5s: scansione `myDesiredRank[0..4]`, primo valido → handshake. Valore segnale = rank-based (1.0→0.2) o 0.1 (fallback). I record sono sempre simmetrici: `RecA->out == RecB->in`.

### Soglie adattive
Se `myDesiredRank` è vuoto per `myThresholdDropDelay`s, `myVisCurrThreshold` scende di `myVisThresholdDropStep`. Promozione retroattiva NotYetDesired → Desired.

### GaveUp
`out ≥ 5 && in < 1` → GaveUp, rimosso da rank.

### Mating trigger
`out ≥ 5 && in ≥ 5` su entrambi i lati → Mating. Punto incontro = punto random in zona Mating più vicina al midpoint. Guard `myCurrentState == Mating` previene double trigger.

### SexBlocked
Permanente. Mai rimosso. Da implementare in U_matingLogic (Branch B).

### Lost
Non visto da >600s → Lost, rimosso da rank. Al re-avvistamento → rivalutazione vs soglia corrente, storico segnali intatto.

---

## Content Browser UE5

```
Content/NPC/
  BP_NPC_Character     ← eredita MY_MOVERTEST (subclass Blueprint di SandboxCharacter_Mover)
                          Ha U_npcBrainComponent aggiunto nel Blueprint Editor
                          Auto Possess AI: Placed in World or Spawned
                          AI Controller Class: BP_NPC_AIController
                          Implementa: BP_Execute* events, BP_ApplyMutableProfile
  BP_NPC_AIController  ← eredita A_npcController
                          AIPerception: sight 2000cm, 60°, tutte le affiliazioni ON
                          OnTargetPerceptionUpdated (per-actor):
                            true  → OnNearbyNPCsUpdated([id])
                            false → OnNpcLostFromSight(id)

Content/Blueprints/
  SandboxCharacter_Mover  ← GASP base character (non modificare)
  MY_MOVERTEST            ← subclass di SandboxCharacter_Mover (locomotion GASP)

Content/Zones/
  A_navMeshZone actors ← ZoneType: Idle o Mating, ZoneShape: Sphere o Box
```

---

## Prossimo step

- **Implementare BP_Execute* in BP_NPC_AIController** — movimento reale con CharacterMover/NavMesh
- **U_matingLogic** — logica post-arrivo zona Mating, Branch A/B, SexBlocked
- **Stato Latency** — cooldown post-mating
- **VIEW 2 (UW_runtimePanel)** — completare pannello runtime
- **Installare plugin Mutable** → decommentare moduli in Build.cs → implementare BP_ApplyMutableProfile

---

## Regole C++ UE5 — critiche

1. **No `TMap`/`TArray` con `{}` initializer lists** come costanti statiche → C-style array
2. **No path subfolder negli include** → `#include "F_npcProfile.h"` non `#include "Population/..."`
3. **`generated.h` sempre ultimo** include in ogni header
4. **`UPROPERTY()`** su ogni `TObjectPtr` e su `TMap<K, UObject*>` per GC
   - `TMap<int32, F_struct>` senza UPROPERTY è corretto (struct plain, non UObject)
   - `TMap<int32, UObject*>` **con** UPROPERTY è obbligatorio (GC deve tracciare i valori)
5. **`GENERATED_BODY()`** in ogni `USTRUCT` / `UCLASS`
6. **RNG deterministico**: `FRandomStream` per profili, `RNG.FRandRange` per varianza runtime
7. **`TWeakObjectPtr`** per riferimenti cross-NPC — previene crash se target distrutto
8. **TMap pointer instability**: non salvare puntatori a campi di struct in TMap — rehash invalida gli indirizzi
9. **`GetGameInstance()` può restituire nullptr**: sempre null-check prima di `->GetSubsystem<>()`
   ```cpp
   if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
       MySubsystem = GI->GetSubsystem<U_mySubsystem>();
   ```
10. **Lambda con `[this]` nei timer**: usare `FTimerDelegate::CreateWeakLambda(this, [...](){ ... })`
    invece di lambda raw — evita UB se `this` viene distrutto prima del timer
11. **`IsValid(Ptr)`** per UObject — non serve `IsValid(Cast<UObject>(Ptr))` se il tipo è già UObject
