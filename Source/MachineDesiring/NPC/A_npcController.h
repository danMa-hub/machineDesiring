#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NpcMemoryTypes.h"
#include "U_npcBrainComponent.h"
#include "U_populationSubsystem.h"
#include "A_npcController.generated.h"

class A_spawnManager;

// ================================================================
// A_npcController — post-refactoring GASP/Mover
//
// RESPONSABILITÀ C++:
//   Percezione   — AIPerception entry point (Blueprint-side)
//   Decisione    — scelta substate, NavMesh destination, timing fasi
//   Segnalazione — handshake rank-based, GaveUp, mating trigger
//   Memoria      — SocialMemory, Lost check, threshold adattivo
//
// RESPONSABILITÀ BLUEPRINT (BP_NPC_AIController):
//   Movimento    — esecuzione fisica (CharacterMover, NavMesh move)
//   Animazione   — Motion Matching, blend space
//   Idle behavior — oscillazione, micro-shift (gestito in BP)
//
// INTERFACCIA C++ → Blueprint:
//   BP_Execute*   = BlueprintImplementableEvent — C++ ordina, BP esegue
//
// INTERFACCIA Blueprint → C++:
//   OnBP*         = BlueprintCallable — BP notifica, C++ decide il prossimo stato
// ================================================================

UCLASS()
class MACHINEDESIRING_API A_npcController : public AAIController
{
	GENERATED_BODY()

public:

	explicit A_npcController(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ── Override AAIController ──────────────────────────────────

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// ── Percezione — entry point Blueprint ─────────────────────

	UPROPERTY(BlueprintReadWrite, Category="NPC|Perception")
	TArray<int32> myNearbyNPCs;

	// Chiamato dal Blueprint (OnTargetPerceptionUpdated, bCurrentlySensed=true)
	UFUNCTION(BlueprintCallable, Category="NPC|Perception")
	void OnNearbyNPCsUpdated(const TArray<int32>& UpdatedIds);

	// Chiamato dal Blueprint (bCurrentlySensed=false)
	UFUNCTION(BlueprintCallable, Category="NPC|Perception")
	void OnNpcLostFromSight(int32 LostId);

	// ── Sub-stato cruising ──────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="NPC|State")
	E_cruisingSubState myCruisingSubState = E_cruisingSubState::RandomWalk;

	// ── Parametri threshold adaptation ─────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Threshold")
	float myThresholdDropDelay    = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Threshold")
	float myVisThresholdDropStep  = 0.05f;

	// ── Callbacks Blueprint → C++ ───────────────────────────────

	// BP chiama questo al termine di ogni fase di movimento (RandomWalk, TargetWalk,
	// IdleWait, MatingWait). C++ sceglie il substate successivo.
	UFUNCTION(BlueprintCallable, Category="NPC|Movement")
	void OnBPPhaseDone();

	// BP chiama questo quando il pawn è arrivato al punto mating.
	UFUNCTION(BlueprintCallable, Category="NPC|Movement")
	void OnBPMatingArrived();

	// BP chiama questo se il path verso il punto mating è fallito.
	// C++ fa retry dopo 2s.
	UFUNCTION(BlueprintCallable, Category="NPC|Movement")
	void OnBPMatingFailed();

	// ── Comandi C++ → Blueprint (movement) ─────────────────────

	// Muovi verso una destinazione random — RandomWalk step.
	// BP esegue il movimento; chiama OnBPPhaseDone quando il pawn arriva.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Movement")
	void BP_ExecuteRandomWalk(FVector Destination);

	// Vai a Location (zona Idle più vicina). Duration = durata sosta in secondi.
	// BaseYaw e PhaseOffset per oscillazione sguardo (da implementare in BP).
	// BP chiama OnBPPhaseDone alla fine della sosta.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Movement")
	void BP_ExecuteIdleWait(FVector Location, float Duration, float BaseYaw, float PhaseOffset);

	// Identico a BP_ExecuteIdleWait ma su zona Mating.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Movement")
	void BP_ExecuteMatingWait(FVector Location, float Duration, float BaseYaw, float PhaseOffset);

	// Insegui TargetId verso Destination. SpeedMult = myMaxWalkSpeed * TARGETWALK_SPEED_MULT.
	// Chiamato ogni 2s (TickTargetWalk) con destinazione aggiornata.
	// BP chiama OnBPPhaseDone quando la fase TargetWalk termina.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Movement")
	void BP_ExecuteTargetWalk(FVector Destination, float SpeedMult, int32 TargetId);

	// Vai verso il punto di mating concordato.
	// BP chiama OnBPMatingArrived o OnBPMatingFailed a seconda del risultato.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Movement")
	void BP_ExecuteMatingMove(FVector Destination);

	// Notifica Blueprint del cambio substate — per aggiornare animazioni.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Movement")
	void BP_OnCruisingSubStateChanged(E_cruisingSubState NewSubState);

	// ── Cross-controller — chiamati dal controller del partner ──

	// Chiamato dopo ogni handshake completato. Gestisce GaveUp e mating trigger.
	void OnSignalExchanged(int32 TargetId, U_npcBrainComponent* TargetBrain);

	// Chiamato dal mating trigger — muovi verso TargetPoint.
	void StartMatingMove(FVector TargetPoint);

	// Getter per overlay inspector — legge il substate corrente
	E_cruisingSubState GetCruisingSubState() const { return myCruisingSubState; }

	// ── Debug ───────────────────────────────────────────────────

	static bool bDebugDrawEnabled;
	static bool bDebugIdEnabled;

private:

	// ── Cache riferimenti ────────────────────────────────────────

	UPROPERTY()
	U_npcBrainComponent* myBrain = nullptr;

	// Pawn posseduto — per GetActorLocation / GetActorForwardVector
	UPROPERTY()
	APawn* myPawn = nullptr;

	UPROPERTY()
	U_populationSubsystem* mySubsystem = nullptr;

	UPROPERTY()
	A_spawnManager* mySpawnManager = nullptr;

	// ── Timer ─────────────────────────────────────────────────

	FTimerHandle myTimer_SignalingLogic;    // 2.5s loop — scan handshake
	FTimerHandle myTimer_LostCheck;         // 300s loop — scansione Lost
	FTimerHandle myTimer_TargetWalkTick;    // 2s loop — aggiorna destinazione TargetWalk
	FTimerHandle myTimer_MatingRetry;       // 2s one-shot — retry mating se path fallisce
	FTimerHandle myTimer_DebugDraw;         // 0.2s loop — label debug sopra pawn
	FTimerHandle myTimer_InitialMovement;   // 0.2s one-shot — startup ritardato

	// ── Stato movimento ──────────────────────────────────────

	// ID target inseguito in TargetWalk. -1 = nessun target.
	int32 myTargetWalkTargetId = -1;

	// Ultima posizione nota del target — fallback NavMesh
	FVector myLastKnownTargetPos = FVector::ZeroVector;

	// Destinazione mating — cachata per i retry
	FVector myMatingDestination = FVector::ZeroVector;

	// Base yaw zona Idle/MatingWait — passato al BP per oscillazione sguardo
	float myIdleBaseYaw     = 0.f;
	float myIdlePhaseOffset = 0.f;

	// ── Accumulatori threshold ────────────────────────────────

	float myTimeWithoutDesired = 0.f;

	// ── Movimento — decisione e NavMesh (esecuzione → Blueprint) ─

	// Distribuzione startup: id%20==0 → MatingWait, id%2==0 → IdleWait, else → RandomWalk
	void StartInitialMovement();

	// Calcola destinazione random su NavMesh (anello [500,2000]cm) → BP_ExecuteRandomWalk
	void StartRandomWalk();

	// Trova zona Idle più vicina → BP_ExecuteIdleWait
	void StartIdleWait();

	// Trova zona Mating più vicina → BP_ExecuteMatingWait
	void StartMatingWait();

	// Helper comune per IdleWait e MatingWait
	void StartZoneWait(E_navMeshZoneType ZoneType, E_cruisingSubState SubState);

	// Sceglie target da top-5 desiredRank → BP_ExecuteTargetWalk
	void StartTargetWalk();

	// Aggiorna destinazione TargetWalk ogni 2s → BP_ExecuteTargetWalk
	void TickTargetWalk();

	// Al termine di ogni fase: sceglie random RandomWalk / IdleWait / TargetWalk
	void ChooseAndStartNextCruisingSubState();

	// ── Segnalazione ─────────────────────────────────────────

	void TickSignalingLogic();

	bool TrySignaling(int32 TargetId, U_npcBrainComponent* TargetBrain);
	void ExchangeSignals(int32 TargetId);
	void ReleaseSignalLock();

	// ── Percezione ───────────────────────────────────────────

	void ProcessPerceivedNpc(int32 TargetId);

	// ── Memoria ──────────────────────────────────────────────

	void TickLostCheck();
	void DropVisThresholdAndPromote();
	void SortDesiredRank();

	// ── NotifyMatingStarted ───────────────────────────────────

	void NotifyMatingStarted(int32 IdA, int32 IdB);

	// ── Debug ────────────────────────────────────────────────

	void TickDebugDraw();
};
