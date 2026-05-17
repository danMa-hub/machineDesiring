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
	TSet<int32> myNearbyNPCs;

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

	// Dot product tra direzione sguardo e direzione movimento (piano 2D).
	//  1.0 = sguardo allineato al movimento
	//  0.0 = sguardo laterale (90°)
	// -1.0 = sguardo opposto al movimento
	// Aggiornato ogni 0.5s in TickGaze. Leggibile dal Blueprint per Stop Movement.
	UPROPERTY(BlueprintReadOnly, Category="NPC|Gaze")
	float myGazeWalkDot = 1.0f;

	// ── Gaze obstacle — tweak in editor senza ricompilare ──────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Gaze")
	float myGazeObstacleEnterDist = 140.f;  // < X cm → P1 Obstacle

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Gaze")
	float myGazeObstacleExitDist  = 180.f;  // > X cm → esce da P1


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

	// Passa il target actor all'AnimBP per il gaze IK.
	// Chiamare solo quando il target cambia.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Gaze")
	void BP_ExecuteGazeLookAt(APawn* TargetPawn);

	// Rimuove il gaze target — blend verso forward neutro.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Gaze")
	void BP_ClearGaze();

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
	static bool bDebugGazeEnabled;

private:

	// ── Sguardo ──────────────────────────────────────────────────

	// Modalità gaze corrente — usata per debug e logica priorità.
	E_gazeMode myCurrentGazeMode = E_gazeMode::None;

	// ID del soggetto attualmente guardato.
	// -1 = nessuno, -2 = player/camera, >= 0 = NPC ID.
	// Cache per evitare aggiornamenti ridondanti all'AnimBP.
	int32 myGazeTargetId = -1;

	// Lista NPC visibili ordinati per VisScore desc — ciclo di scansione sguardo.
	TArray<int32> myGazeScanList;

	// Indice corrente nel ciclo myGazeScanList.
	int32 myGazeScanIndex = 0;

	// P1 Obstacle: poll ogni 0.5s — rileva ostacoli < 140cm e uscita isteresi.
	void TickGaze();

	// Aggiorna myGazeWalkDot ogni 1s — solo se in movimento, altrimenti resetta a 1.0.
	void TickGazeWalkCheck();


	// Avanza al prossimo NPC nel ciclo di scansione e schedula il timer per la sua durata.
	void AdvanceGazeScan();

	// Ricostruisce myGazeScanList da myNearbyNPCs, ordinata per VisScore desc.
	// Chiamata ogni volta che myNearbyNPCs cambia.
	void RebuildGazeScanList();

	// Durata dello sguardo verso Id: proporzionale a VisScore.
	// Blacklist → 0.1–0.3s. VisScore [0,1] → lerp [0.2s, 3.0s].
	float GazeDurationForId(int32 Id) const;

	// Aggiorna gaze solo se mode/target sono cambiati — evita chiamate BP ridondanti.
	void SetGaze(E_gazeMode NewMode, int32 NewId, APawn* NewPawn);

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
	FTimerHandle myTimer_GazeTick;          // 0.5s loop — P1 obstacle detection
	FTimerHandle myTimer_GazeScan;          // one-shot variabile — avanza ciclo scansione
	FTimerHandle myTimer_GazeRebuild;       // 2.0s loop — rebuild lista prossimità
	FTimerHandle myTimer_GazeWalkCheck;     // 1.0s loop — aggiorna myGazeWalkDot
	FTimerHandle myTimer_ReleaseSignal;     // 1.5s one-shot — rilascia lock segnalazione

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
