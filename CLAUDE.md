# CLAUDE.md
# Macchina del Desiderio — UE5 C++ Reference

---

## Progetto

Installazione artistica interattiva in UE5.7. 700 NPC autonomi simulano dinamiche relazionali e sessuali della comunità gay maschile. Omaggio alla Macchina Celibe di Duchamp.

L'utente sta imparando C++ in UE5. Spiega ogni passo chiaramente. Implementa un passo alla volta. Compila prima di procedere.

**Repository:** `D:/THEGUTS/theGutsUnreal/MachineDesiring/MachineDesiring/`

**IDE:** Visual Studio Code + MSVC BuildTools 2022

**Build:**
```
VS Code:    Ctrl+Shift+B  →  "Build MachineDesiringEditor (Development)"  [default]
Claude:     UnrealBuildTool.exe MachineDesiringEditor Win64 Development -Project="...uproject"
Editor:     Tools > Compile (solo modifiche minori, hot reload)
```
Dopo C++: chiudi editor UE5 → build → riapri. Hot reload funziona solo per modifiche minori.

---

## Struttura file

```
Source/MachineDesiring/
  Population/
    F_npcProfile.h/.cpp          ← profilo NPC + enum + pesi
    U_populationSubsystem.h/.cpp ← 700 profili + matrici attrazione

  NPC/
    NpcMemoryTypes.h             ← enum e struct condivisi (non modificare)
    A_navMeshZone.h/.cpp         ← zone Idle/Mating sul NavMesh
    A_npcCharacter.h/.cpp        ← character base: identità, FSM, memoria sociale
    A_npcController.h/.cpp       ← logica cruising: percezione, movimento, segnalazione, mating
    A_npcController.h/.cpp       ← logica cruising: segnalazione, soglie, Lost
    A_playerController.h/.cpp    ← toggle debug (tasto 1 = overlay, tasto 2 = ID)
    A_spawnManager.h/.cpp        ← spawn subset da pool 700, NpcById TMap
```

**Build.cs dipendenze:**
```
Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, NavigationSystem
```
`PublicIncludePaths` copre tutte le sottocartelle — usa include flat nei .cpp.

---

## Naming convention — OBBLIGATORIA

### Prefissi tipo UE5
| Prefisso | Tipo | Esempio |
|---|---|---|
| `A_` | Actor | `A_npcCharacter`, `A_spawnManager` |
| `U_` | UObject/Subsystem | `U_populationSubsystem` |
| `F_` | Struct | `F_npcProfile`, `F_otherIDMemory` |
| `E_` | Enum | `E_myState`, `E_npcEthnicity` |

### Variabili runtime NPC
| Prefisso | Scope | Esempio |
|---|---|---|
| `My` PascalCase | tratto profilo (`F_npcProfile`) | `MyMuscle`, `MyAge` |
| `my` camelCase | variabile runtime per-NPC | `myCurrentState`, `mySocialMemory` |
| `myOut_` | score/segnale uscente | `myOut_signalValue` |
| `myIn_` | score/segnale entrante | `myIn_signalValue` |
| `b_my` | bool per-NPC | `b_myIsSignaling` |
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

## Stato implementazione

### U_populationSubsystem
- 700 `F_npcProfile` generati con seed deterministico (Box-Muller, FRandomStream)
- `VisAttractionMatrix` / `SexAttractionMatrix` N×N pre-calcolate
- `GetVisScore(ObsId, TgtId)` / `GetSexScore(ObsId, TgtId)` — O(1)
- `GetMeshScaleZ(NpcId)` — scala Z [0.5, 4.0] normalizzata su appeal visivo

### A_npcCharacter
```cpp
int32                              myId
E_myState                          myCurrentState      // Cruising / Mating / Latency
TMap<int32, F_otherIDMemory>       mySocialMemory      // no UPROPERTY
TArray<int32>                      myDesiredRank       // max 20, ordinato per VisScore desc
TSet<int32>                        myStaticVisBlacklist
bool                               b_myIsSignaling
TWeakObjectPtr<A_npcCharacter>     mySignalTarget
float  myVisCurrThreshold / myVisMinThreshold / myVisStartThreshold
float  mySexCurrThreshold / mySexMinThreshold / mySexStartThreshold
float  myMaxWalkSpeed       // [70, 100] cm/s, deterministico per-NPC
float  myRotationRate       // 60°/s default
// Materiali per sub-stato (assegnabili in BP_NPC_Character):
UMaterialInterface* Mat_RandomWalk / Mat_Idle / Mat_Mating / Mat_TargetWalk
```
- `myVisMinThreshold`: gauss(0.10, 0.02) clamp [0.05, 0.20], seed = myId
- `mySexMinThreshold`: 0.40 fisso
- `SetSubStateMaterial`: itera tutti gli `USkeletalMeshComponent` (non solo `GetMesh()`)

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
WALK_DURATION_MIN/MAX    = 30 / 90 s

// TargetWalk
TARGETWALK_RADIUS        = 150 cm   // raggio punto random attorno al target
TARGETWALK_FORWARD_OFFSET= 300 cm   // offset avanti inseguitore (supera il target)
TARGETWALK_SPEED_MULT    = 1.8f
TARGETWALK_DURATION_MIN/MAX = 50 / 120 s

// Idle
IDLE_DURATION_MIN/MAX    = 10 / 55 s
IDLE_SHIFT_RADIUS        = 80 cm
IDLE_SEPARATION_WEIGHT   = 0.15f    // bassa → folla compatta
WALK_SEPARATION_WEIGHT   = 1.0f

// Stuck
STUCK_MIN_DISTANCE       = 40 cm in 2s
```

**Percezione (Blueprint-side):**
- AIPerception con sight cone (2000cm, 60°) su `BP_NPC_AIController`
- Evento: `OnTargetPerceptionUpdated` (per-actor, non batch)
  - bCurrentlySensed=true → `OnNearbyNPCsUpdated([id])`
  - bCurrentlySensed=false → `OnNpcLostFromSight(id)`
- `ProcessPerceivedNpc`: blacklist O(1) → TMap.Find O(1) → VisScore → rank

**Movimento (E_cruisingSubState):**
```
StartInitialMovement (0.2s dopo OnPossess):
  id%20==0 → MatingWait (5%)
  id%2==0  → IdleWait  (~45%)
  else     → RandomWalk (~50%)

RandomWalk:  anello [500,2000]cm, pre-rotazione, durata [30,90]s
IdleWait:    zona Idle più vicina, sosta [10,55]s, oscillazione sguardo sin wave,
             micro-shift 80cm ogni 2s (mantiene crowd agent attivo)
MatingWait:  identico IdleWait ma su zone Mating
TargetWalk:  insegue top-5 desiderati (solo Cruising), vel 1.8×,
             offset 300cm avanti, TickTargetWalk 2s loop, durata [50,120]s

ChooseAndStartNextCruisingSubState: 3-way random al termine di ogni fase
OnMoveCompleted: dispatcher — Mating retry | RandomWalk/TargetWalk loop | Idle arrivo | shift ciclo
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
3. **Sincronizzazione forzata:** `RecA->myIn = RecB->myOut` e `RecB->myIn = RecA->myOut`
   → garantisce simmetria indipendentemente da chi inizia lo scambio
4. Debug line: verde se reciproco, rosso se unilaterale
5. `OnSignalExchanged(A)` → se A non GaveUp → `OnSignalExchanged(B)`

**OnSignalExchanged:**
- Guard: stato già terminale → return (evita log duplicati)
- GaveUp: out ≥ 5 && in < 1 → GaveUp, rimosso da rank
- Mating trigger: out ≥ 5 && in ≥ 5 su entrambi i lati → SetState(Mating) su entrambi
  → `NotifyMatingStarted` → `StartMatingMove` (stesso punto per entrambi)

**ReleaseSignalLock:** non resetta b_myIsSignaling su NPC in stato Mating.

**Mating:**
- `NotifyMatingStarted(IdA, IdB)`: marca GaveUp in tutti gli NPC Cruising che avevano IdA/IdB nel rank
- `StartMatingMove(Point)`: clears TUTTI i timer cruising, MoveToLocation(Point, 50cm)
  - path fallito → retry dopo 2s via myTimer_MatingRetry
- `StartTargetWalk` / `TickTargetWalk`: skippano target non in Cruising
- `StartMatingMove` e `OnSignalExchanged` sono **public** (chiamate cross-controller)

**Memoria:**
- `TickLostCheck` ogni 300s: Lost se `Now - myTLastSeen > 600s`
- `DropVisThresholdAndPromote`: drop soglia + promozione retroattiva NotYetDesired → Desired
- `SortDesiredRank`: sort per VisScore desc, chiamato dopo ogni Add

**Debug:**
- `bDebugDrawEnabled` (tasto 1): label sub-stato colorato sopra NPC
- `bDebugIdEnabled` (tasto 2): ID overlay, mostra "ID → targetID" in TargetWalk
- Linea cyan: destinazione TargetWalk (sempre visibile)
- Timer DebugDraw: 0.2s loop → `TickDebugDraw`

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
`out ≥ 5 && in < 1` → GaveUp, rimosso da rank. Guard in OnSignalExchanged previene doppi effetti.

### Mating trigger
`out ≥ 5 && in ≥ 5` su entrambi i lati → Mating. Punto incontro = punto random in zona Mating più vicina al midpoint.

### SexBlocked
Permanente. Mai rimosso. Da implementare in U_matingLogic (Branch B).

### Lost
Non visto da >600s → Lost, rimosso da rank. Al re-avvistamento → rivalutazione vs soglia corrente, storico segnali intatto.

---

## Content Browser UE5

```
Content/NPC/
  BP_NPC_Character     ← eredita A_npcCharacter
                          Auto Possess AI: Placed in World or Spawned
                          AI Controller Class: BP_NPC_AIController
  BP_NPC_AIController  ← eredita A_npcController
                          AIPerception: sight 2000cm, 60°, tutte le affiliazioni ON
                          OnTargetPerceptionUpdated (per-actor):
                            true  → OnNearbyNPCsUpdated([id])
                            false → OnNpcLostFromSight(id)

Content/Zones/
  A_navMeshZone actors ← ZoneType: Idle o Mating, ZoneShape: Sphere o Box
```

---

## Prossimo step

- **U_matingLogic**: logica post-arrivo in zona Mating, Branch A/B, SexBlocked
- **Stato Latency**: cooldown post-mating
- **NotifyMatingStarted**: segnata TODO, rivisitare dopo U_matingLogic

---

## Regole C++ UE5 — critiche

1. **No `TMap`/`TArray` con `{}` initializer lists** come costanti statiche → C-style array
2. **No path subfolder negli include** → `#include "F_npcProfile.h"` non `#include "Population/..."`
3. **`generated.h` sempre ultimo** include in ogni header
4. **`UPROPERTY()`** su ogni `TObjectPtr` per GC
5. **`GENERATED_BODY()`** in ogni `USTRUCT` / `UCLASS`
6. **RNG deterministico**: `FRandomStream` per profili, `RNG.FRandRange` per varianza runtime
7. **`TMap<int32, F_otherIDMemory>`** senza `UPROPERTY` — corretto, `int32` non è UObject
8. **`TWeakObjectPtr`** per riferimenti cross-NPC — previene crash se target distrutto
9. **TMap pointer instability**: non salvare puntatori a campi di struct in TMap — rehash invalida gli indirizzi
