#pragma once

#include "CoreMinimal.h"
#include "NpcMemoryTypes.generated.h"

// ================================================================
// NpcMemoryTypes.h
// Tipi condivisi per il sistema Cruising & Mating.
// Nessuna logica — solo enums e structs.
// Incluso da: A_npcCharacter, A_npcController,
//             U_cruisingLogic, U_matingLogic
// ================================================================


// ----------------------------------------------------------------
// E_myState
// Stato FSM principale dell'NPC.
// Gestito da A_npcCharacter, letto da A_npcController.
// ----------------------------------------------------------------
UENUM(BlueprintType)
enum class E_myState : uint8
{
    Cruising    UMETA(DisplayName = "Cruising"),  // default — cerca partner
    Mating      UMETA(DisplayName = "Mating"),    // si muove verso zona / accoppiamento
    Latency     UMETA(DisplayName = "Latency"),   // cooldown post-mating o punto morto
};


// ----------------------------------------------------------------
// E_cruisingSubState
// Sotto-stato interno al Cruising.
// Vive in A_npcController — non è parte della FSM principale.
// ----------------------------------------------------------------
UENUM(BlueprintType)
enum class E_cruisingSubState : uint8
{
    RandomWalk  UMETA(DisplayName = "RandomWalk"),   // naviga punto random NavMesh
    IdleWait    UMETA(DisplayName = "IdleWait"),     // sosta in A_navMeshZone tipo Idle
    MatingWait  UMETA(DisplayName = "MatingWait"),  // sosta in A_navMeshZone tipo Mating
    TargetWalk  UMETA(DisplayName = "TargetWalk"),  // si avvicina al primo ID in myDesiredRank
};


// ----------------------------------------------------------------
// E_otherIDState
// Stato di un entry in mySocialMemory.
// Rappresenta la relazione dell'NPC verso un altro ID specifico.
// ----------------------------------------------------------------
UENUM(BlueprintType)
enum class E_otherIDState : uint8
{
    Desired         UMETA(DisplayName = "Desired"),         // above threshold — in myDesiredRank
    NotYetDesired   UMETA(DisplayName = "NotYetDesired"),   // below threshold — storico registrato
    GaveUp          UMETA(DisplayName = "GaveUp"),          // rinuncia per inefficacia sociale
    Lost            UMETA(DisplayName = "Lost"),            // non visto da >600s — record congelato
    SexBlocked      UMETA(DisplayName = "SexBlocked"),      // blacklist sessuale — permanente, impermeabile
};


// ----------------------------------------------------------------
// E_navMeshZoneType
// Tipo di zona nel livello.
// Usato da A_navMeshZone per distinguere zone Idle e Mating.
// ----------------------------------------------------------------
UENUM(BlueprintType)
enum class E_navMeshZoneType : uint8
{
    Idle    UMETA(DisplayName = "Idle"),    // sosta durante cruising
    Mating  UMETA(DisplayName = "Mating"), // zona accoppiamento
};


// ----------------------------------------------------------------
// F_otherIDMemory
// Entry singola in mySocialMemory.
// Contiene tutto ciò che un NPC sa/sente riguardo a un altro NPC.
//
// NOTA myOut_visAttraction / myOut_sexAttraction NON sono qui:
// si leggono direttamente dalla matrice pre-calcolata in
// U_populationSubsystem via GetVisScore()/GetSexScore() — O(1).
// Duplicarli sarebbe ridondante.
//
// NOTA SexBlocked è impermeabile — check SEMPRE prioritario
// rispetto a qualsiasi altra valutazione (vedi U_cruisingLogic).
// ----------------------------------------------------------------
USTRUCT(BlueprintType)
struct F_otherIDMemory
{
    GENERATED_BODY()

    // ID dell'NPC a cui si riferisce questo record
    UPROPERTY(BlueprintReadOnly)
    int32 targetId = -1;

    // Stato corrente della relazione verso questo ID
    // SexBlocked = early return in qualsiasi valutazione
    UPROPERTY(BlueprintReadOnly)
    E_otherIDState state = E_otherIDState::NotYetDesired;

    // Segnali inviati verso questo ID (accumulati)
    // Fonte: rank-based signaling (Algoritmo Cruising & Mating §4.1)
    UPROPERTY(BlueprintReadOnly)
    float myOut_signalValue = 0.f;

    UPROPERTY(BlueprintReadOnly)
    int32 myOut_signalCount = 0;

    // Segnali ricevuti da questo ID (accumulati)
    // Usati per trigger mating e per GaveUp check
    UPROPERTY(BlueprintReadOnly)
    float myIn_signalValue = 0.f;

    UPROPERTY(BlueprintReadOnly)
    int32 myIn_signalCount = 0;

    // Timestamp ultimo avvistamento — GetWorld()->GetTimeSeconds()
    // Usato per: CurrentTime - myTLastSeen > 600s → Lost
    UPROPERTY(BlueprintReadOnly)
    float myTLastSeen = 0.f;
};
