#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NpcMemoryTypes.h"
#include "A_npcCharacter.h"
#include "U_populationSubsystem.h"
#include "A_npcController.generated.h"

class A_spawnManager;

// ================================================================
// A_npcController
// Gestisce tutta la logica runtime dell'NPC:
//   Percezione  — entry point Blueprint (AIPerception + sight cone)
//   Movimento   — RandomWalk / IdleWait / MatingWait / TargetWalk
//   Segnalazione — handshake rank-based, GaveUp, mating trigger
//   Memoria     — SocialMemory, Lost check, threshold adattivo
// ================================================================

UCLASS()
class MACHINEDESIRING_API A_npcController : public AAIController
{
	GENERATED_BODY()

public:

	explicit A_npcController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ── Override AAIController ───────────────────────────────────

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	// ── NPC vicini — popolato dal Blueprint (AIPerception lato BP) ──

	// Lista ID NPC attualmente nel cone visivo
	// Aggiornata dal Blueprint via OnNearbyNPCsUpdated()
	UPROPERTY(BlueprintReadWrite, Category="NPC|Perception")
	TArray<int32> myNearbyNPCs;

	// Chiamato dal Blueprint ogni volta che AIPerception aggiorna
	// il set di NPC visibili. Entry point per aggiornare mySocialMemory.
	UFUNCTION(BlueprintCallable, Category="NPC|Perception")
	void OnNearbyNPCsUpdated(const TArray<int32>& UpdatedIds);

	// Chiamato dal Blueprint quando un NPC esce dal cono visivo.
	// Aggiorna solo myTLastSeen — storico segnali e stato intatti.
	UFUNCTION(BlueprintCallable, Category="NPC|Perception")
	void OnNpcLostFromSight(int32 LostId);

	// ── Entry point post-segnalazione (event-driven) ─────────────

	// Chiamato da ExchangeSignals dopo ogni handshake completato.
	// Esegue: guard stato terminale → GaveUp check → mating trigger.
	// Deve essere public: il controller di A lo chiama anche sul controller di B.
	void OnSignalExchanged(int32 TargetId, A_npcCharacter* TargetChar);

	// Chiamato dal mating trigger sul controller del partner.
	// Deve essere public: il controller di A lo chiama sul controller di B.
	void StartMatingMove(FVector TargetPoint);

	// Toggle globale overlay debug — premere "1" in gioco (A_playerController)
	static bool bDebugDrawEnabled;

	// Toggle overlay ID NPC — premere "2" in gioco
	static bool bDebugIdEnabled;

	// ── Sotto-stato movimento (interno al Cruising) ──────────────

	UPROPERTY(BlueprintReadOnly, Category="NPC|State")
	E_cruisingSubState myCruisingSubState = E_cruisingSubState::RandomWalk;

	// ── Parametri threshold adaptation ──────────────────────────

	// Secondi senza nessun Desired prima di abbassare la soglia vis
	// Editabile per variare selettività individuale
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Threshold")
	float myThresholdDropDelay = 60.f;

	// Step di discesa per ogni drop (default 0.05)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Threshold")
	float myVisThresholdDropStep = 0.05f;

private:

	// ── Cache riferimenti ────────────────────────────────────────

	// Pawn posseduto — cachato in OnPossess, azzerato in OnUnPossess
	UPROPERTY()
	A_npcCharacter* myCharacter = nullptr;

	// Subsystem — cachato in OnPossess per evitare GetSubsystem ogni call
	UPROPERTY()
	U_populationSubsystem* mySubsystem = nullptr;

	// SpawnManager — cachato in OnPossess per lookup O(1) NpcById
	UPROPERTY()
	A_spawnManager* mySpawnManager = nullptr;

	// ── Timer ────────────────────────────────────────────────────

	FTimerHandle myTimer_SignalingLogic;    // 2.5s — scan handshake
	FTimerHandle myTimer_LostCheck;         // 300s — scansione Lost su mySocialMemory
	FTimerHandle myTimer_PreRotation;       // one-shot — attende fine rotazione pre-walk
	FTimerHandle myTimer_StuckCheck;        // 2.0s — rileva NPC bloccati
	FTimerHandle myTimer_IdleWait;          // [10s,180s] one-shot — sosta in zona Idle
	FTimerHandle myTimer_IdleOscillation;   // 0.1s loop — oscillazione sguardo in Idle
	FTimerHandle myTimer_IdleShift;         // [4s,8s] one-shot — micro-spostamento crowd
	FTimerHandle myTimer_DebugDraw;         // 0.2s loop — label debug sopra capsula
	FTimerHandle myTimer_StuckVisualReset;  // 3s one-shot — rimuove materiale rosso stuck
	FTimerHandle myTimer_ShiftTimeout;          // 1.5s one-shot — forza fine shift se NPC bloccato dalla folla
	FTimerHandle myTimer_RandomWalkDuration;    // [40s,200s] one-shot — durata fase RandomWalk
	FTimerHandle myTimer_TargetWalkDuration;    // [40s,200s] one-shot — durata fase TargetWalk
	FTimerHandle myTimer_TargetWalkTick;        // 2s loop — aggiorna destinazione sul target corrente
	FTimerHandle myTimer_MatingRetry;           // 2s one-shot — retry MoveToLocation se path Mating fallito

	// Dati oscillazione Idle — calcolati in StartIdleWait, usati in TickIdleOscillation
	float myIdleBaseYaw    = 0.f;  // yaw zona + offset random ±40°
	float myIdlePhaseOffset = 0.f; // fase random — evita sincronismo tra NPC

	// Flag: true quando NPC è arrivato nella zona Idle (sosta attiva)
	bool b_myIdleArrived = false;

	// Flag: true durante i 3s successivi a uno stuck detection (per overlay + materiale)
	bool b_myIsStuck = false;

	// Flag: true mentre è in corso un micro-shift (idle o stuck recovery)
	bool b_myIsShifting = false;

	// Posizione al momento dello stuck — confrontata in OnStuckVisualExpired
	FVector myStuckAtPosition = FVector::ZeroVector;

	// Destinazione calcolata in StartRandomWalk, usata da StartMoveAfterRotation
	FVector myPendingDestination     = FVector::ZeroVector;
	FVector myLastStuckCheckPosition = FVector::ZeroVector;

	// Destinazione mating — cachata per i retry
	FVector myMatingDestination = FVector::ZeroVector;

	// ID del target attualmente inseguito in TargetWalk.
	// -1 = nessun target → al prossimo StartTargetWalk ne viene scelto uno nuovo.
	// Resettato a -1 quando il target viene raggiunto o da ChooseAndStartNextCruisingSubState.
	int32   myTargetWalkTargetId   = -1;

	// Ultima posizione valida del target — usata come fallback se GetRandomReachablePoint fallisce.
	FVector myLastKnownTargetPos   = FVector::ZeroVector;

	// ── Accumulatori threshold ───────────────────────────────────

	// Secondi accumulati senza nessun entry Desired in myDesiredRank
	float myTimeWithoutDesired = 0.f;

	// ── Tick periodico cruising (ogni 2.5s) ──────────────────────

	// Ordine interno:
	//   guard b_myIsSignaling → return
	//   guard state != Cruising → return
	//   loop myDesiredRank[0..4]:
	//     check disponibilità (bool) — cheapest
	//     check distanza (vector)
	//     check FOV reciproco (dot product) — expensive
	//   primo valido → TrySignaling → break
	//   aggiorna myTimeWithoutDesired → eventuale threshold drop
	// Chiamato 0.2s dopo OnPossess — avvia 50/50 startup dopo BeginPlay delle zone.
	void StartInitialMovement();

	// Orienta il character verso Destination, poi avvia MoveToLocation via myTimer_PreRotation.
	// Helper condiviso da StartRandomWalk e StartIdleWait.
	void BeginPreRotationToward(FVector Destination);

	// Calcola una destinazione random, orienta il character, poi avvia il movimento.
	// Chiamato una volta in OnPossess, poi da OnIdleWaitExpired.
	void StartRandomWalk();

	// Trova la zona Idle più vicina, ottiene un punto random al suo interno, muove lì.
	// Al termine (OnMoveCompleted) si ferma poi torna a RandomWalk.
	void StartIdleWait();

	// Identica a StartIdleWait ma punta alle zone Mating.
	void StartMatingWait();

	// Sceglie un punto random vicino a uno dei 5 target di desiderio e cammina lì.
	// Se myDesiredRank è vuoto, fallback a StartRandomWalk.
	void StartTargetWalk();

	// Al termine di un sotto-stato, sceglie casualmente il prossimo tra i 3 disponibili.
	void ChooseAndStartNextCruisingSubState();

	// Chiamato dopo sosta casuale [10s,180s] → ferma oscillazione → StartRandomWalk.
	void OnIdleWaitExpired();

	// Timer 0.1s: aggiorna SetControlRotation con sin wave attorno a myIdleBaseYaw.
	void TickIdleOscillation();

	// Timer 0.2s: DrawDebugString sopra la capsula con substate corrente.
	void TickDebugDraw();

	// Timer [4s,8s] one-shot: micro-spostamento in zona Idle, mantiene CrowdAgent attivo.
	void TickIdleShift();

	// Timer 3s one-shot: controlla recovery stuck; se ancora fermo → StartRandomWalk.
	void OnStuckVisualExpired();

	// Timer 1.5s one-shot: scatta se lo shift non completa in tempo (folla densa).
	// Forza fine shift e schedula la prossima pausa 2s.
	void OnShiftTimeout();

	// Timer [40s,200s] one-shot: scatta quando la fase RandomWalk è durata abbastanza.
	// Il passaggio avviene al prossimo OnMoveCompleted (timer inattivo = segnale).
	void OnRandomWalkDurationExpired();

	// Identico per TargetWalk.
	void OnTargetWalkDurationExpired();

	// 2s loop: aggiorna destinazione sul target corrente mentre in TargetWalk.
	void TickTargetWalk();

	// Chiamato dal timer pre-rotazione — ripristina bOrientRotationToMovement e avvia MoveToLocation.
	void StartMoveAfterRotation();

	// Chiamato ogni 1s — se l'NPC si è mosso meno di 20cm è bloccato → nuovo target.
	void TickStuckCheck();

	void TickSignalingLogic();

	// ── Handshake ────────────────────────────────────────────────

	// Tenta handshake atomico con TargetId.
	// Restituisce true se handshake avvenuto.
	// Blocca b_myIsSignaling su entrambi gli NPC.
	bool TrySignaling(int32 TargetId, A_npcCharacter* TargetChar);

	// Esegue lo scambio di segnali rank-based tra A e B.
	// Chiamato internamente da TrySignaling dopo lock.
	// Valori inviati: rank 0→1.0, 1→0.8, 2→0.6, 3→0.4, 4→0.2, >4→0.1
	void ExchangeSignals(int32 TargetId);

	// Rilascia il lock handshake su entrambi gli NPC.
	// Chiamato su timer 1.5s dopo ExchangeSignals.
	void ReleaseSignalLock();

	// ── Helpers percezione ───────────────────────────────────────

	// Aggiorna mySocialMemory per un NPC appena visto:
	//   blacklist check → Desired / NotYetDesired → myTLastSeen
	void ProcessPerceivedNpc(int32 TargetId);


	// ── Tick Lost check (ogni 300s) ────────────────────────────

	// Scansiona mySocialMemory — se currentTime - myTLastSeen > 600s → Lost
	// Rimuove da myDesiredRank se presente.
	void TickLostCheck();

	// ── Helpers rank ────────────────────────────────────────

	// Ordina myDesiredRank per VisScore decrescente — chiamato dopo ogni Add
	void SortDesiredRank();

	// ── Helpers threshold ────────────────────────────────────────

	// Abbassa myVisCurrThreshold di myVisThresholdDropStep.
	// Poi scansiona mySocialMemory per promozioni retroattive.
	void DropVisThresholdAndPromote();

	// Chiamato al trigger mating: marca GaveUp in tutti gli NPC in Cruising
	// che avevano IdA o IdB nella propria myDesiredRank.
	// TODO: rivisitare quando la logica mating sarà completa.
	void NotifyMatingStarted(int32 IdA, int32 IdB);
};
