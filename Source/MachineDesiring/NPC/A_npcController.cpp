// Fill out your copyright notice in the Description page of Project Settings.

#include "A_npcController.h"
#include "A_spawnManager.h"
#include "A_navMeshZone.h"
#include "U_overlaySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

// ================================================================
// Costanti — raggruppate per area funzionale
// ================================================================

// ── Segnalazione (Algoritmo_Cruising §4–6) ───────────────────────
static constexpr float SIGNAL_ACTIVE_DURATION   = 1.5f;
static constexpr float SIGNAL_VALUES[]          = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f };
static constexpr float SIGNAL_VALUE_FALLBACK    = 0.1f;   // rank > 4
static constexpr float GAVEUP_SENT_THRESHOLD    = 5.0f;
static constexpr float GAVEUP_RECV_THRESHOLD    =  1.0f;
static constexpr float MATING_SIGNAL_THRESHOLD  = 5.0f;
static constexpr float LOST_TIMEOUT_SECONDS     = 600.0f;
static constexpr float SIGNALING_TIMER_INTERVAL = 2.5f;

// ── RandomWalk ───────────────────────────────────────────────────
static constexpr float WALK_RADIUS_MIN    = 500.f;   // cm — anello interno  (5m)
static constexpr float WALK_RADIUS_MAX    = 2000.f;  // cm — anello esterno (20m)
static constexpr float WALK_ACCEPTANCE_R  = 150.f;   // cm — soglia arrivo
static constexpr float WALK_DURATION_MIN  = 30.f;    // s  — durata minima fase RandomWalk
static constexpr float WALK_DURATION_MAX  = 90.f;    // s  — durata massima fase RandomWalk

// ── TargetWalk ───────────────────────────────────────────────────
static constexpr float TARGETWALK_RADIUS         = 150.f;  // cm — raggio punto random attorno al target
static constexpr float TARGETWALK_FORWARD_OFFSET = 300.f;  // cm — offset avanti inseguitore (supera il target)
static constexpr float TARGETWALK_REACH_DIST     = 150.f;  // cm — distanza per "target raggiunto"
static constexpr float TARGETWALK_SPEED_MULT     = 1.8f;   // moltiplicatore velocità
static constexpr float TARGETWALK_DURATION_MIN   = 50.f;   // s  — durata minima fase
static constexpr float TARGETWALK_DURATION_MAX   = 120.f;  // s  — durata massima fase

// ── Idle — durata sosta ──────────────────────────────────────────
static constexpr float IDLE_DURATION_MIN = 10.f;    // s  — durata minima sosta
static constexpr float IDLE_DURATION_MAX = 55.f;   // s  — durata massima sosta

// ── Idle — oscillazione sguardo ──────────────────────────────────
static constexpr float IDLE_OSC_AMPLITUDE = 30.f;   // ±30° attorno alla base yaw
static constexpr float IDLE_OSC_FREQUENCY = 0.8f;   // rad/s — ciclo ~8s
static constexpr float IDLE_OSC_TICK      = 0.1f;   // s — refresh SetControlRotation
static constexpr float IDLE_YAW_OFFSET    = 40.f;   // ° — offset random ±40° per-NPC

// ── Idle — micro-shift crowd ─────────────────────────────────────
static constexpr float IDLE_SHIFT_RADIUS   = 80.f;  // cm — raggio micro-spostamento
static constexpr float IDLE_SHIFT_ACCEPT_R = 10.f;  // cm — soglia arrivo
static constexpr float IDLE_SHIFT_PAUSE    = 2.f;   // s  — pausa tra uno shift e il successivo
static constexpr float IDLE_SHIFT_TIMEOUT  = 1.5f;  // s  — timeout se la folla blocca lo shift

// ── Crowd separation ─────────────────────────────────────────────
static constexpr float IDLE_SEPARATION_WEIGHT = 0.15f;  // bassa → folla compatta
static constexpr float WALK_SEPARATION_WEIGHT = 1.0f;   // piena → separazione normale

// ── Stuck recovery ───────────────────────────────────────────────
static constexpr float STUCK_MIN_DISTANCE = 40.f;   // cm in 2s — sotto = bloccato

bool A_npcController::bDebugDrawEnabled = false;
bool A_npcController::bDebugIdEnabled   = false;

// ================================================================
// Costruttore
// ================================================================

A_npcController::A_npcController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = false;
}

// ================================================================
// OnPossess / OnUnPossess
// ================================================================

void A_npcController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Detour Crowd — agente già registrato dopo Super::OnPossess
	if (UCrowdFollowingComponent* CrowdComp =
		Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdComp->SetCrowdSeparation(true, true);
		CrowdComp->SetCrowdSeparationWeight(WALK_SEPARATION_WEIGHT, true);
		CrowdComp->SetCrowdCollisionQueryRange(200.f, true);
		CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Medium, true);
	}

	myCharacter = Cast<A_npcCharacter>(InPawn);
	if (!myCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("A_npcController: OnPossess — pawn non è A_npcCharacter"));
		return;
	}

	mySubsystem = UGameplayStatics::GetGameInstance(this)->GetSubsystem<U_populationSubsystem>();
	if (!mySubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("A_npcController %d: U_populationSubsystem non trovato"),
			myCharacter->myId);
		return;
	}

	// Cache SpawnManager — TActorIterator una volta sola, non ogni tick
	for (TActorIterator<A_spawnManager> It(GetWorld()); It; ++It)
	{
		mySpawnManager = *It;
		break;
	}

	// Timer persistenti (loop)
	GetWorldTimerManager().SetTimer(myTimer_SignalingLogic, this,
		&A_npcController::TickSignalingLogic, SIGNALING_TIMER_INTERVAL, true);
	GetWorldTimerManager().SetTimer(myTimer_LostCheck, this,
		&A_npcController::TickLostCheck, 300.f, true);
	GetWorldTimerManager().SetTimer(myTimer_StuckCheck, this,
		&A_npcController::TickStuckCheck, 2.0f, true);
	GetWorldTimerManager().SetTimer(myTimer_DebugDraw, this,
		&A_npcController::TickDebugDraw, 0.2f, true);

	myLastStuckCheckPosition = myCharacter->GetActorLocation();

	// Startup ritardato di 0.2s — garantisce che tutte le zone Idle abbiano
	// completato BeginPlay e si siano registrate in allIdleZones prima della
	// prima chiamata a StartIdleWait.
	GetWorldTimerManager().SetTimer(myTimer_PreRotation, this,
		&A_npcController::StartInitialMovement, 0.2f, false);
}

void A_npcController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(myTimer_SignalingLogic);
	GetWorldTimerManager().ClearTimer(myTimer_LostCheck);
	GetWorldTimerManager().ClearTimer(myTimer_PreRotation);
	GetWorldTimerManager().ClearTimer(myTimer_StuckCheck);
	GetWorldTimerManager().ClearTimer(myTimer_StuckVisualReset);
	GetWorldTimerManager().ClearTimer(myTimer_RandomWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);
	GetWorldTimerManager().ClearTimer(myTimer_IdleWait);
	GetWorldTimerManager().ClearTimer(myTimer_IdleOscillation);
	GetWorldTimerManager().ClearTimer(myTimer_IdleShift);
	GetWorldTimerManager().ClearTimer(myTimer_ShiftTimeout);
	GetWorldTimerManager().ClearTimer(myTimer_MatingRetry);
	GetWorldTimerManager().ClearTimer(myTimer_DebugDraw);
	StopMovement();

	myCharacter    = nullptr;
	mySubsystem    = nullptr;
	mySpawnManager = nullptr;

	Super::OnUnPossess();
}

// ================================================================
// Percezione
// Entry point: Blueprint chiama OnNearbyNPCsUpdated dopo AIPerception.
// ================================================================

void A_npcController::OnNearbyNPCsUpdated(const TArray<int32>& UpdatedIds)
{
	if (!myCharacter || !mySubsystem) return;

	for (int32 Id : UpdatedIds)
	{
		myNearbyNPCs.AddUnique(Id);
		ProcessPerceivedNpc(Id);
	}

	SortDesiredRank();
}

void A_npcController::OnNpcLostFromSight(int32 LostId)
{
	if (!myCharacter) return;

	myNearbyNPCs.Remove(LostId);

	// Aggiorna solo il timestamp — storico segnali e stato relazione intatti
	if (F_otherIDMemory* Rec = myCharacter->mySocialMemory.Find(LostId))
		Rec->myTLastSeen = GetWorld()->GetTimeSeconds();
}

void A_npcController::ProcessPerceivedNpc(int32 TargetId)
{
	if (!myCharacter || !mySubsystem) return;
	if (TargetId == myCharacter->myId) return;

	// 1. Blacklist statica — O(1), prima di tutto
	if (myCharacter->myStaticVisBlacklist.Contains(TargetId)) return;

	const float Now = GetWorld()->GetTimeSeconds();

	// 2. Già in memoria — O(1)
	if (F_otherIDMemory* Rec = myCharacter->mySocialMemory.Find(TargetId))
	{
		if (Rec->state == E_otherIDState::SexBlocked) return;

		Rec->myTLastSeen = Now;

		if (Rec->state == E_otherIDState::Lost)
		{
			const float VisScore = mySubsystem->GetVisScore(myCharacter->myId, TargetId);
			Rec->state = (VisScore >= myCharacter->myVisCurrThreshold)
			             ? E_otherIDState::Desired : E_otherIDState::NotYetDesired;

			if (Rec->state == E_otherIDState::Desired
				&& !myCharacter->myDesiredRank.Contains(TargetId)
				&& myCharacter->myDesiredRank.Num() < 20)
			{
				myCharacter->myDesiredRank.Add(TargetId);
			}
		}
		return;
	}

	// 3. Prima volta — calcola VisScore solo ora
	const float VisScore = mySubsystem->GetVisScore(myCharacter->myId, TargetId);

	if (VisScore < myCharacter->myVisMinThreshold)
	{
		myCharacter->myStaticVisBlacklist.Add(TargetId);
		return;
	}

	F_otherIDMemory NewRec;
	NewRec.targetId    = TargetId;
	NewRec.myTLastSeen = Now;
	NewRec.state = (VisScore >= myCharacter->myVisCurrThreshold)
	               ? E_otherIDState::Desired : E_otherIDState::NotYetDesired;

	myCharacter->mySocialMemory.Add(TargetId, NewRec);

	if (NewRec.state == E_otherIDState::Desired && myCharacter->myDesiredRank.Num() < 20)
		myCharacter->myDesiredRank.Add(TargetId);
}

// ================================================================
// Movimento — ciclo RandomWalk ↔ IdleWait
//
//   StartRandomWalk  →  BeginPreRotationToward  →  StartMoveAfterRotation
//     →  OnMoveCompleted  →  StartIdleWait
//     →  BeginPreRotationToward  →  StartMoveAfterRotation
//     →  OnMoveCompleted [primo arrivo]  →  sosta [10s,180s]
//     →  OnIdleWaitExpired  →  StartRandomWalk  →  ...
// ================================================================

// Chiamato 0.2s dopo OnPossess — tutte le zone hanno completato BeginPlay.
// Distribuzione deterministica con myId:
//   id % 20 == 0  →  5%  MatingWait
//   id % 2  == 0  → ~45% IdleWait
//   else          → ~50% RandomWalk
void A_npcController::StartInitialMovement()
{
	if (!myCharacter) return;
	const int32 Id = myCharacter->myId;
	if (Id % 20 == 0)
	{
		StartMatingWait();
	}
	else if (Id % 2 == 0)
	{
		StartIdleWait();
	}
	else
	{
		GetWorldTimerManager().SetTimer(myTimer_RandomWalkDuration, this,
			&A_npcController::OnRandomWalkDurationExpired,
			FMath::RandRange(WALK_DURATION_MIN, WALK_DURATION_MAX), false);
		StartRandomWalk();
	}
}

// Helper condiviso: orienta il character verso Destination,
// poi avvia MoveToLocation tramite myTimer_PreRotation.
void A_npcController::BeginPreRotationToward(FVector Destination)
{
	myPendingDestination = Destination;

	FVector Dir = (Destination - myCharacter->GetActorLocation());
	Dir.Z = 0.f;
	Dir.Normalize();
	const FRotator TargetRot(0.f, Dir.Rotation().Yaw, 0.f);
	const float AngleDiff = FMath::Abs(
		FMath::FindDeltaAngleDegrees(myCharacter->GetActorRotation().Yaw, TargetRot.Yaw));

	if (UCharacterMovementComponent* MoveComp = myCharacter->GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement    = false;
		MoveComp->bUseControllerDesiredRotation = true;
	}
	SetControlRotation(TargetRot);

	const float RotTime = FMath::Max(AngleDiff / myCharacter->myRotationRate, 0.05f);
	GetWorldTimerManager().SetTimer(myTimer_PreRotation, this,
		&A_npcController::StartMoveAfterRotation, RotTime, false);
}

void A_npcController::StartRandomWalk()
{
	if (!myCharacter) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	myCruisingSubState = E_cruisingSubState::RandomWalk;
	b_myIdleArrived    = false;
	myCharacter->SetSubStateMaterial(E_cruisingSubState::RandomWalk);
	if (UCharacterMovementComponent* M = myCharacter->GetCharacterMovement())
		M->MaxWalkSpeed = myCharacter->myMaxWalkSpeed;

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return;

	const FVector Origin     = myCharacter->GetActorLocation();
	const float   RandomAngle = FMath::RandRange(0.f, 2.f * UE_PI);
	const FVector RandomDir(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.f);

	// Campionamento uniforme sull'area anulare — evita concentrazione al centro
	const float Dist = FMath::Sqrt(FMath::RandRange(
		WALK_RADIUS_MIN * WALK_RADIUS_MIN,
		WALK_RADIUS_MAX * WALK_RADIUS_MAX));

	FNavLocation NavLocation;
	if (!NavSys->ProjectPointToNavigation(
		Origin + RandomDir * Dist, NavLocation,
		FVector(WALK_RADIUS_MAX, WALK_RADIUS_MAX, WALK_RADIUS_MAX)))
	{
		// Fallback: punto raggiungibile qualunque
		if (!NavSys->GetRandomReachablePointInRadius(Origin, WALK_RADIUS_MAX, NavLocation))
			return;
	}

	BeginPreRotationToward(NavLocation.Location);
}

void A_npcController::StartIdleWait()
{
	if (!myCharacter) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	myCruisingSubState = E_cruisingSubState::IdleWait;
	b_myIdleArrived    = false;
	myCharacter->SetSubStateMaterial(E_cruisingSubState::IdleWait);
	if (UCharacterMovementComponent* M = myCharacter->GetCharacterMovement())
		M->MaxWalkSpeed = myCharacter->myMaxWalkSpeed;

	GetWorldTimerManager().ClearTimer(myTimer_RandomWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);

	A_navMeshZone* Zone = A_navMeshZone::GetNearest(
		GetWorld(), myCharacter->GetActorLocation(), E_navMeshZoneType::Idle);

	if (!Zone)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC %d: nessuna zona Idle — fallback RandomWalk"),
			myCharacter->myId);
		StartRandomWalk();
		return;
	}

	FVector TargetPoint;
	if (!Zone->GetRandomNavPoint(TargetPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC %d: GetRandomNavPoint fallito — fallback RandomWalk"),
			myCharacter->myId);
		StartRandomWalk();
		return;
	}

	// Base yaw = direzione freccia zona + offset random ±40° per-NPC
	myIdleBaseYaw    = Zone->arrowComp->GetComponentRotation().Yaw
	                 + FMath::RandRange(-IDLE_YAW_OFFSET, IDLE_YAW_OFFSET);
	myIdlePhaseOffset = FMath::RandRange(0.f, 2.f * UE_PI);

	BeginPreRotationToward(TargetPoint);
}

void A_npcController::StartMatingWait()
{
	if (!myCharacter) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	myCruisingSubState = E_cruisingSubState::MatingWait;
	b_myIdleArrived    = false;
	myCharacter->SetSubStateMaterial(E_cruisingSubState::MatingWait);

	GetWorldTimerManager().ClearTimer(myTimer_RandomWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);

	A_navMeshZone* Zone = A_navMeshZone::GetNearest(
		GetWorld(), myCharacter->GetActorLocation(), E_navMeshZoneType::Mating);

	if (!Zone)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC %d: nessuna zona Mating — fallback RandomWalk"),
			myCharacter->myId);
		StartRandomWalk();
		return;
	}

	FVector TargetPoint;
	if (!Zone->GetRandomNavPoint(TargetPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC %d: GetRandomNavPoint Mating fallito — fallback RandomWalk"),
			myCharacter->myId);
		StartRandomWalk();
		return;
	}

	myIdleBaseYaw     = Zone->arrowComp->GetComponentRotation().Yaw
	                  + FMath::RandRange(-IDLE_YAW_OFFSET, IDLE_YAW_OFFSET);
	myIdlePhaseOffset = FMath::RandRange(0.f, 2.f * UE_PI);

	BeginPreRotationToward(TargetPoint);
}

void A_npcController::StartTargetWalk()
{
	if (!myCharacter || !mySpawnManager) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	// Sub-stato e tick attivati subito — anche se NavMesh fallisce il tick riprova
	myCruisingSubState = E_cruisingSubState::TargetWalk;
	b_myIdleArrived    = false;
	myCharacter->SetSubStateMaterial(E_cruisingSubState::TargetWalk);
	if (UCharacterMovementComponent* M = myCharacter->GetCharacterMovement())
		M->MaxWalkSpeed = myCharacter->myMaxWalkSpeed * TARGETWALK_SPEED_MULT;
	if (!GetWorldTimerManager().IsTimerActive(myTimer_TargetWalkTick))
	{
		GetWorldTimerManager().SetTimer(myTimer_TargetWalkTick, this,
			&A_npcController::TickTargetWalk, 2.0f, true);
	}

	if (myCharacter->myDesiredRank.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[TARGET] NPC %d: desiredRank vuoto, attendo prossimo tick"), myCharacter->myId);
		return;
	}

	// ── Scegli o mantieni il target ───────────────────────────────
	if (myTargetWalkTargetId != -1)
	{
		A_npcCharacter* PrevChar = mySpawnManager->NpcById.FindRef(myTargetWalkTargetId);
		if (!PrevChar || !IsValid(PrevChar))
		{
			UE_LOG(LogTemp, Log, TEXT("[TARGET LOST] NPC %d: target %d non trovato → nuovo"),
				myCharacter->myId, myTargetWalkTargetId);
			myTargetWalkTargetId = -1;
		}
		else if (PrevChar->myCurrentState != E_myState::Cruising)
		{
			UE_LOG(LogTemp, Log, TEXT("[TARGET LOST] NPC %d: target %d non più in Cruising → nuovo"),
				myCharacter->myId, myTargetWalkTargetId);
			myTargetWalkTargetId = -1;
		}
		else if (FVector::Dist2D(myCharacter->GetActorLocation(),
		                         PrevChar->GetActorLocation()) <= TARGETWALK_REACH_DIST)
		{
			UE_LOG(LogTemp, Log, TEXT("[TARGET REACHED] NPC %d → target %d raggiunto"),
				myCharacter->myId, myTargetWalkTargetId);
			myTargetWalkTargetId = -1;
		}
	}

	// Nuovo target se necessario — sceglie tra i top-5, salta chi non è in Cruising
	const bool bNewTarget = (myTargetWalkTargetId == -1);
	if (bNewTarget)
	{
		const int32 MaxIdx  = FMath::Min(myCharacter->myDesiredRank.Num(), 5) - 1;
		const int32 StartIdx = FMath::RandRange(0, MaxIdx);
		for (int32 i = 0; i <= MaxIdx; ++i)
		{
			const int32 CandidateId = myCharacter->myDesiredRank[(StartIdx + i) % (MaxIdx + 1)];
			A_npcCharacter* Candidate = mySpawnManager->NpcById.FindRef(CandidateId);
			if (Candidate && IsValid(Candidate)
				&& Candidate->myCurrentState == E_myState::Cruising)
			{
				myTargetWalkTargetId = CandidateId;
				break;
			}
		}
		if (myTargetWalkTargetId == -1)
		{
			UE_LOG(LogTemp, Log, TEXT("[TARGET] NPC %d: nessun target Cruising nel rank, attendo"),
				myCharacter->myId);
			return;
		}
	}

	A_npcCharacter* TargetChar = mySpawnManager->NpcById.FindRef(myTargetWalkTargetId);
	if (!TargetChar || !IsValid(TargetChar))
	{
		UE_LOG(LogTemp, Log, TEXT("[TARGET LOST] NPC %d: char %d non valido, attendo prossimo tick"),
			myCharacter->myId, myTargetWalkTargetId);
		myTargetWalkTargetId = -1;
		return;   // tick riprova tra 2s
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return;

	FNavLocation NavLocation;
	if (!NavSys->GetRandomReachablePointInRadius(
			TargetChar->GetActorLocation(), TARGETWALK_RADIUS, NavLocation))
	{
		UE_LOG(LogTemp, Log, TEXT("[TARGET] NPC %d: target %d fuori NavMesh, attendo prossimo tick"),
			myCharacter->myId, myTargetWalkTargetId);
		return;   // tick riprova tra 2s — NON fallback RandomWalk
	}

	// Log solo quando viene selezionato un nuovo target
	if (bNewTarget)
	{
		const float Dist = FVector::Dist(myCharacter->GetActorLocation(), TargetChar->GetActorLocation());
		UE_LOG(LogTemp, Log,
			TEXT("[TARGET] NPC %d → nuovo target=%d | dist=%.0fcm"),
			myCharacter->myId, myTargetWalkTargetId, Dist);
	}

	BeginPreRotationToward(NavLocation.Location);
}

void A_npcController::TickTargetWalk()
{
	if (!myCharacter || !mySpawnManager) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;
	if (myCruisingSubState != E_cruisingSubState::TargetWalk) return;
	if (myTargetWalkTargetId == -1) return;

	A_npcCharacter* TargetChar = mySpawnManager->NpcById.FindRef(myTargetWalkTargetId);
	if (!TargetChar || !IsValid(TargetChar))
	{
		UE_LOG(LogTemp, Log, TEXT("[TARGET LOST] NPC %d: target %d non trovato in NpcById"),
			myCharacter->myId, myTargetWalkTargetId);
		myTargetWalkTargetId = -1;
		return;
	}

	// Se il target è uscito dal Cruising (entrato in Mating/Latency) smetti di seguirlo
	if (TargetChar->myCurrentState != E_myState::Cruising)
	{
		UE_LOG(LogTemp, Log, TEXT("[TARGET LOST] NPC %d: target %d non più in Cruising → stop inseguimento"),
			myCharacter->myId, myTargetWalkTargetId);
		myTargetWalkTargetId = -1;
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return;

	// Aggiorna ultima posizione nota del target
	const FVector CurrentTargetPos = TargetChar->GetActorLocation();

	// Offset avanti: l'inseguitore punta oltre il target, affiancandosi o superandolo.
	const FVector ChaseOrigin = CurrentTargetPos
	    + myCharacter->GetActorForwardVector() * TARGETWALK_FORWARD_OFFSET;

	// Ricalcola punto random attorno alla posizione CORRENTE (con offset)
	FNavLocation NavLocation;
	if (NavSys->GetRandomReachablePointInRadius(ChaseOrigin, TARGETWALK_RADIUS, NavLocation))
	{
		myLastKnownTargetPos = CurrentTargetPos;
		myPendingDestination = NavLocation.Location;
		MoveToLocation(NavLocation.Location, WALK_ACCEPTANCE_R, false);
		return;
	}

	// Fallback: usa l'ultima posizione nota se quella corrente non è sulla NavMesh
	UE_LOG(LogTemp, Log,
		TEXT("[TARGET LOST] NPC %d: target %d fuori NavMesh (pos=%.0f,%.0f,%.0f) → fallback ultima pos nota"),
		myCharacter->myId, myTargetWalkTargetId,
		CurrentTargetPos.X, CurrentTargetPos.Y, CurrentTargetPos.Z);

	const FVector FallbackOrigin = myLastKnownTargetPos
	    + myCharacter->GetActorForwardVector() * TARGETWALK_FORWARD_OFFSET;
	if (myLastKnownTargetPos != FVector::ZeroVector
		&& NavSys->GetRandomReachablePointInRadius(FallbackOrigin, TARGETWALK_RADIUS, NavLocation))
	{
		myPendingDestination = NavLocation.Location;
		MoveToLocation(NavLocation.Location, WALK_ACCEPTANCE_R, false);
		return;
	}

	// Nessun punto trovato — attende il prossimo tick senza cambiare stato
}

// Durata fase TargetWalk scaduta — il passaggio avviene al prossimo OnMoveCompleted.
void A_npcController::OnTargetWalkDurationExpired() { }

// Al termine di un sotto-stato di camminata, sceglie casualmente il prossimo tra i 3.
// Chiamato da OnMoveCompleted (durata scaduta) e da OnIdleWaitExpired.
void A_npcController::ChooseAndStartNextCruisingSubState()
{
	if (!myCharacter) return;

	if (myCruisingSubState == E_cruisingSubState::TargetWalk)
		UE_LOG(LogTemp, Log, TEXT("[TARGET] NPC %d: durata scaduta → cambio stato"), myCharacter->myId);

	const int32 Roll = FMath::RandRange(0, 2);

	if (Roll == 0)
	{
		GetWorldTimerManager().SetTimer(myTimer_RandomWalkDuration, this,
			&A_npcController::OnRandomWalkDurationExpired,
			FMath::RandRange(WALK_DURATION_MIN, WALK_DURATION_MAX), false);
		StartRandomWalk();
	}
	else if (Roll == 1)
	{
		StartIdleWait();
	}
	else
	{
		if (myCharacter->myDesiredRank.Num() > 0)
		{
			myTargetWalkTargetId = -1;   // nuovo ciclo → nuovo target
			GetWorldTimerManager().SetTimer(myTimer_TargetWalkDuration, this,
				&A_npcController::OnTargetWalkDurationExpired,
				FMath::RandRange(TARGETWALK_DURATION_MIN, TARGETWALK_DURATION_MAX), false);
			StartTargetWalk();
		}
		else
		{
			// Nessun target → RandomWalk
			GetWorldTimerManager().SetTimer(myTimer_RandomWalkDuration, this,
				&A_npcController::OnRandomWalkDurationExpired,
				FMath::RandRange(WALK_DURATION_MIN, WALK_DURATION_MAX), false);
			StartRandomWalk();
		}
	}
}

void A_npcController::StartMoveAfterRotation()
{
	if (!myCharacter) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	if (UCharacterMovementComponent* MoveComp = myCharacter->GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement    = true;
		MoveComp->bUseControllerDesiredRotation = false;
	}

	MoveToLocation(myPendingDestination, WALK_ACCEPTANCE_R, false);
}

// ================================================================
// OnMoveCompleted — dispatcher principale
//
//   Mating (path fallito)         →  retry dopo 2s
//   RandomWalk / TargetWalk       →  stessa fase (durata attiva) | ChooseNext (scaduta)
//   Avvicinamento a zona Idle/Mating:
//     path fallito                →  riprova (StartIdleWait / StartMatingWait)
//     primo arrivo                →  setup sosta, oscillazione, timer shift
//   Ritorno da micro-shift        →  pausa IDLE_SHIFT_PAUSE → prossimo shift
// ================================================================

void A_npcController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!myCharacter) return;

	// ── Mating: retry se path fallito ────────────────────────────
	if (myCharacter->myCurrentState == E_myState::Mating)
	{
		if (Result.Code != EPathFollowingResult::Success
			&& myMatingDestination != FVector::ZeroVector)
		{
			// Riprova dopo 2s — la folla potrebbe bloccare temporaneamente il path
			GetWorldTimerManager().SetTimer(myTimer_MatingRetry, [this]()
			{
				if (myCharacter && myCharacter->myCurrentState == E_myState::Mating
					&& myMatingDestination != FVector::ZeroVector)
				{
					UE_LOG(LogTemp, Log, TEXT("[MATING RETRY] NPC %d → (%.0f,%.0f,%.0f)"),
						myCharacter->myId,
						myMatingDestination.X, myMatingDestination.Y, myMatingDestination.Z);
					MoveToLocation(myMatingDestination, 50.f, false);
				}
			}, 2.0f, false);
		}
		return;
	}

	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	// ── RandomWalk o TargetWalk completato ───────────────────────
	if (myCruisingSubState == E_cruisingSubState::RandomWalk
		|| myCruisingSubState == E_cruisingSubState::TargetWalk)
	{
		b_myIsShifting = false;

		const bool bDurationActive =
			(myCruisingSubState == E_cruisingSubState::RandomWalk)
			? GetWorldTimerManager().IsTimerActive(myTimer_RandomWalkDuration)
			: GetWorldTimerManager().IsTimerActive(myTimer_TargetWalkDuration);

		if (bDurationActive)
		{
			// Durata in corso → nuova destinazione nello stesso sotto-stato
			if (myCruisingSubState == E_cruisingSubState::RandomWalk)
				StartRandomWalk();
			else
				StartTargetWalk();
		}
		else
		{
			// Durata scaduta → scelta random del prossimo sotto-stato
			ChooseAndStartNextCruisingSubState();
		}
		return;
	}

	// ── In cammino verso la zona Idle o Mating ────────────────────
	if (!b_myIdleArrived)
	{
		// Path fallito → riprova nella zona appropriata
		if (Result.Code != EPathFollowingResult::Success)
		{
			if (myCruisingSubState == E_cruisingSubState::MatingWait)
				StartMatingWait();
			else
				StartIdleWait();
			return;
		}

		// Primo arrivo reale nella zona
		b_myIdleArrived = true;
		myCharacter->SetSubStateMaterial(myCruisingSubState);

		if (UCrowdFollowingComponent* CrowdComp =
			Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
		{
			CrowdComp->SetCrowdSeparationWeight(IDLE_SEPARATION_WEIGHT, true);
		}

		if (UCharacterMovementComponent* MoveComp = myCharacter->GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement    = false;
			MoveComp->bUseControllerDesiredRotation = true;
		}
		SetControlRotation(FRotator(0.f, myIdleBaseYaw, 0.f));

		// Oscillazione sguardo
		GetWorldTimerManager().SetTimer(myTimer_IdleOscillation, this,
			&A_npcController::TickIdleOscillation, IDLE_OSC_TICK, true);

		// Durata sosta [20s, 120s] → poi StartRandomWalk
		const float IdleDuration = FMath::RandRange(IDLE_DURATION_MIN, IDLE_DURATION_MAX);
		GetWorldTimerManager().SetTimer(myTimer_IdleWait, this,
			&A_npcController::OnIdleWaitExpired, IdleDuration, false);

		// Primo micro-shift dopo pausa iniziale
		GetWorldTimerManager().SetTimer(myTimer_IdleShift, this,
			&A_npcController::TickIdleShift, IDLE_SHIFT_PAUSE, false);

		UE_LOG(LogTemp, Verbose, TEXT("NPC %d: %s — durata=%.0fs"),
			myCharacter->myId,
			myCruisingSubState == E_cruisingSubState::MatingWait ? TEXT("MATING_OK") : TEXT("IDLE_OK"),
			IdleDuration);
		return;
	}

	// ── Ritorno da micro-shift → pausa → prossimo shift ──────────
	GetWorldTimerManager().ClearTimer(myTimer_ShiftTimeout);
	b_myIsShifting = false;
	GetWorldTimerManager().SetTimer(myTimer_IdleShift, this,
		&A_npcController::TickIdleShift, IDLE_SHIFT_PAUSE, false);
}

// ================================================================
// Idle behavior — sosta, micro-shift, oscillazione sguardo
// ================================================================

// Sosta scaduta → ripristina stato walk → StartRandomWalk
void A_npcController::OnIdleWaitExpired()
{
	if (!myCharacter) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	b_myIdleArrived = false;
	b_myIsShifting  = false;

	GetWorldTimerManager().ClearTimer(myTimer_IdleShift);
	GetWorldTimerManager().ClearTimer(myTimer_ShiftTimeout);
	GetWorldTimerManager().ClearTimer(myTimer_IdleOscillation);

	if (UCrowdFollowingComponent* CrowdComp =
		Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdComp->SetCrowdSeparationWeight(WALK_SEPARATION_WEIGHT, true);
	}
	if (UCharacterMovementComponent* MoveComp = myCharacter->GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement    = true;
		MoveComp->bUseControllerDesiredRotation = false;
	}

	ChooseAndStartNextCruisingSubState();
}

// Micro-spostamento 80cm con pathfinding → Detour Crowd attivo →
// separation forces permettono redistribuzione della folla.
// Ciclo: TickIdleShift → MoveToLocation → OnMoveCompleted → pausa 2s → TickIdleShift
void A_npcController::TickIdleShift()
{
	if (!myCharacter) return;
	if (!b_myIdleArrived) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	b_myIsShifting = false;   // safety reset — riprende anche se OnMoveCompleted non è scattato

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return;

	FNavLocation ShiftLocation;
	if (NavSys->GetRandomReachablePointInRadius(
			myCharacter->GetActorLocation(), IDLE_SHIFT_RADIUS, ShiftLocation))
	{
		b_myIsShifting = true;
		MoveToLocation(ShiftLocation.Location, IDLE_SHIFT_ACCEPT_R,
			false, true, true, false, nullptr, false);
		// Timeout: se OnMoveCompleted non arriva entro IDLE_SHIFT_TIMEOUT → forza fine shift
		GetWorldTimerManager().SetTimer(myTimer_ShiftTimeout, this,
			&A_npcController::OnShiftTimeout, IDLE_SHIFT_TIMEOUT, false);
	}
	else
	{
		// NavMesh non trovato → riprova dopo la pausa standard
		GetWorldTimerManager().SetTimer(myTimer_IdleShift, this,
			&A_npcController::TickIdleShift, IDLE_SHIFT_PAUSE, false);
	}
}

// Scatta se lo shift non completa entro IDLE_SHIFT_TIMEOUT (NPC bloccato dalla folla)
void A_npcController::OnShiftTimeout()
{
	if (!myCharacter) return;
	if (!b_myIdleArrived) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	StopMovement();
	b_myIsShifting = false;
	GetWorldTimerManager().SetTimer(myTimer_IdleShift, this,
		&A_npcController::TickIdleShift, IDLE_SHIFT_PAUSE, false);
}

// Durata fase RandomWalk scaduta — il passaggio a IdleWait avviene
// al prossimo OnMoveCompleted (timer inattivo = segnale di transizione)
void A_npcController::OnRandomWalkDurationExpired() { }

// Sin wave ±30° attorno a myIdleBaseYaw — aggiornato a 10 Hz
void A_npcController::TickIdleOscillation()
{
	if (!myCharacter) return;
	const float t = GetWorld()->GetTimeSeconds();
	SetControlRotation(FRotator(0.f,
		myIdleBaseYaw + FMath::Sin(t * IDLE_OSC_FREQUENCY + myIdlePhaseOffset) * IDLE_OSC_AMPLITUDE,
		0.f));
}

// ================================================================
// Stuck recovery
// ================================================================

// Ogni 2s: se NPC si è mosso meno di 40cm → stuck.
// Prima tenta micro-shift 150cm; se ancora fermo dopo 3s → StartRandomWalk.
void A_npcController::TickStuckCheck()
{
	if (!myCharacter) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;
	if (b_myIsStuck)   return;   // già in recovery
	if (b_myIdleArrived) return; // idle: lo shift gestisce tutto
	if (GetWorldTimerManager().IsTimerActive(myTimer_PreRotation)) return;
	if (GetWorldTimerManager().IsTimerActive(myTimer_IdleWait))    return;

	const FVector CurrentPos = myCharacter->GetActorLocation();
	const float   DistMoved  = FVector::Dist2D(CurrentPos, myLastStuckCheckPosition);

	if (DistMoved < STUCK_MIN_DISTANCE)
	{
		b_myIsStuck       = true;
		myStuckAtPosition = CurrentPos;

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		FNavLocation ShiftLoc;
		if (NavSys && NavSys->GetRandomReachablePointInRadius(CurrentPos, 150.f, ShiftLoc))
		{
			b_myIsShifting = true;
			MoveToLocation(ShiftLoc.Location, 10.f, false, true, true, false, nullptr, false);
		}

		GetWorldTimerManager().SetTimer(myTimer_StuckVisualReset, this,
			&A_npcController::OnStuckVisualExpired, 3.f, false);

		UE_LOG(LogTemp, Warning, TEXT("NPC %d: stuck (%.1fcm) → micro-shift"),
			myCharacter->myId, DistMoved);
	}

	myLastStuckCheckPosition = CurrentPos;
}

// Dopo 3s: se l'NPC non si è spostato → StartRandomWalk
void A_npcController::OnStuckVisualExpired()
{
	b_myIsStuck    = false;
	b_myIsShifting = false;

	if (!myCharacter) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;
	if (b_myIdleArrived) return;

	if (FVector::Dist2D(myCharacter->GetActorLocation(), myStuckAtPosition) < STUCK_MIN_DISTANCE)
	{
		StopMovement();
		StartRandomWalk();
	}
}

// ================================================================
// Segnalazione
// ================================================================

void A_npcController::TickSignalingLogic()
{
	if (!myCharacter || !mySubsystem) return;
	if (myCharacter->b_myIsSignaling) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	const int32 MaxScan = FMath::Min(myCharacter->myDesiredRank.Num(), 5);
	for (int32 i = 0; i < MaxScan; i++)
	{
		const int32 TargetId = myCharacter->myDesiredRank[i];

		F_otherIDMemory* Rec = myCharacter->mySocialMemory.Find(TargetId);
		if (!Rec) continue;
		if (Rec->state == E_otherIDState::SexBlocked
			|| Rec->state == E_otherIDState::GaveUp
			|| Rec->state == E_otherIDState::Lost) continue;

		// Check 1 — disponibilità
		if (!mySpawnManager) continue;
		A_npcCharacter* TargetChar = mySpawnManager->NpcById.FindRef(TargetId);
		if (!TargetChar || !IsValid(TargetChar)) continue;
		if (TargetChar->b_myIsSignaling) continue;
		if (TargetChar->myCurrentState != E_myState::Cruising) continue;

		// Check 2 — distanza ≤ 400cm
		if (FVector::DistSquared(myCharacter->GetActorLocation(),
			TargetChar->GetActorLocation()) > 400.f * 400.f) continue;

		// Check 3 — FOV reciproco wide (soglia -0.5 ≈ cono 120°, 240° totale per NPC)
		// Copre: frontale, affiancamento, inseguimento, superamento.
		// Escluso solo il cono cieco di 60° direttamente dietro la testa.
		const FVector DirAtoB = (TargetChar->GetActorLocation()
		                         - myCharacter->GetActorLocation()).GetSafeNormal();
		if (FVector::DotProduct(myCharacter->GetActorForwardVector(),  DirAtoB) < -0.2f) continue;
		if (FVector::DotProduct(TargetChar->GetActorForwardVector(), -DirAtoB) < -0.2f) continue;

		if (TrySignaling(TargetId, TargetChar)) break;
	}

	// Threshold accumulator — drop soglia se nessun Desired da troppo tempo
	if (myCharacter->myDesiredRank.Num() == 0)
	{
		myTimeWithoutDesired += SIGNALING_TIMER_INTERVAL;
		if (myTimeWithoutDesired >= myThresholdDropDelay)
		{
			myTimeWithoutDesired = 0.f;
			DropVisThresholdAndPromote();
		}
	}
	else
	{
		myTimeWithoutDesired = 0.f;
	}
}

bool A_npcController::TrySignaling(int32 TargetId, A_npcCharacter* TargetChar)
{
	if (!myCharacter || !TargetChar || !IsValid(TargetChar)) return false;
	if (myCharacter->b_myIsSignaling || TargetChar->b_myIsSignaling) return false;

	// Lock atomico su entrambi
	myCharacter->b_myIsSignaling = true;
	myCharacter->mySignalTarget  = TargetChar;
	TargetChar->b_myIsSignaling  = true;
	TargetChar->mySignalTarget   = myCharacter;

	ExchangeSignals(TargetId);

	FTimerHandle ReleaseTimer;
	GetWorldTimerManager().SetTimer(ReleaseTimer,
		FTimerDelegate::CreateUObject(this, &A_npcController::ReleaseSignalLock),
		SIGNAL_ACTIVE_DURATION, false);

	return true;
}

void A_npcController::ExchangeSignals(int32 TargetId)
{
	if (!myCharacter || !mySubsystem) return;

	A_npcCharacter* TargetChar = myCharacter->mySignalTarget.Get();
	if (!TargetChar) return;

	// Valore A→B: rank di B in myDesiredRank di A
	const int32 RankA   = myCharacter->myDesiredRank.Find(TargetId);
	const float SentByA = (RankA >= 0 && RankA < 5) ? SIGNAL_VALUES[RankA] : SIGNAL_VALUE_FALLBACK;

	// Valore B→A: rank di A in myDesiredRank di B (0 se A è sotto soglia per B)
	const int32 RankB = TargetChar->myDesiredRank.Find(myCharacter->myId);
	float SentByB = 0.f;
	if      (RankB >= 0 && RankB < 5) SentByB = SIGNAL_VALUES[RankB];
	else if (RankB >= 5)               SentByB = SIGNAL_VALUE_FALLBACK;

	F_otherIDMemory* RecA = myCharacter->mySocialMemory.Find(TargetId);
	F_otherIDMemory* RecB = TargetChar->mySocialMemory.Find(myCharacter->myId);

	// Aggiorna out di chi invia (se non in stato terminale)
	if (RecA && RecA->state != E_otherIDState::GaveUp && RecA->state != E_otherIDState::SexBlocked)
	{
		RecA->myOut_signalValue += SentByA;  RecA->myOut_signalCount++;
	}
	if (RecB && RecB->state != E_otherIDState::GaveUp && RecB->state != E_otherIDState::SexBlocked)
	{
		RecB->myOut_signalValue += SentByB;  RecB->myOut_signalCount++;
	}

	// Sincronizzazione forzata: in = out dell'altro lato.
	// Garantisce RecA->out == RecB->in e RecB->out == RecA->in
	// indipendentemente da chi ha iniziato lo scambio e dalla frequenza.
	if (RecA && RecB)
	{
		RecA->myIn_signalValue = RecB->myOut_signalValue;
		RecB->myIn_signalValue = RecA->myOut_signalValue;
		RecA->myIn_signalCount = RecB->myOut_signalCount;
		RecB->myIn_signalCount = RecA->myOut_signalCount;
	}

	// ── Log + debug line scambio segnali ────────────────────────
	if (RecA)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[SIGNAL] NPC %d → %d | inviato=%.1f (tot_out=%.1f) | ricevuto=%.1f (tot_in=%.1f)"),
			myCharacter->myId, TargetId,
			SentByA, RecA->myOut_signalValue,
			SentByB, RecA->myIn_signalValue);
	}
	if (RecB)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[SIGNAL] NPC %d → %d | inviato=%.1f (tot_out=%.1f) | ricevuto=%.1f (tot_in=%.1f)"),
			TargetId, myCharacter->myId,
			SentByB, RecB->myOut_signalValue,
			SentByA, RecB->myIn_signalValue);
	}

	// Linea debug: verde se segnale ricambiato (SentByB > 0), rosso se unilaterale.
	// Spessore proporzionale all'intensità del segnale inviato da A.
	{
		const FVector PosA = myCharacter->GetActorLocation() + FVector(0.f, 0.f, 90.f);
		const FVector PosB = TargetChar->GetActorLocation()  + FVector(0.f, 0.f, 90.f);
		const FColor  LineColor   = (SentByB > 0.f) ? FColor::Green : FColor::Red;
		const float   LineThickness = SentByA * 10.f;   // range: 0.4 (fallback) → 4.0 (rank 0)
		DrawDebugLine(GetWorld(), PosA, PosB, LineColor, false, 1.0f, 0, LineThickness);
	}

	OnSignalExchanged(TargetId, TargetChar);

	// Notifica B solo se A non ha appena fatto GaveUp
	F_otherIDMemory* CheckRecA = myCharacter->mySocialMemory.Find(TargetId);
	if (!(CheckRecA && CheckRecA->state == E_otherIDState::GaveUp))
	{
		if (A_npcController* TargetCtrl = Cast<A_npcController>(TargetChar->GetController()))
			TargetCtrl->OnSignalExchanged(myCharacter->myId, myCharacter);
	}
}

void A_npcController::ReleaseSignalLock()
{
	if (!myCharacter) return;

	A_npcCharacter* TargetChar = myCharacter->mySignalTarget.Get();
	if (TargetChar && IsValid(TargetChar)
		&& TargetChar->myCurrentState != E_myState::Mating)
	{
		TargetChar->b_myIsSignaling = false;
		TargetChar->mySignalTarget  = nullptr;
	}

	if (myCharacter->myCurrentState != E_myState::Mating)
	{
		myCharacter->b_myIsSignaling = false;
		myCharacter->mySignalTarget  = nullptr;
	}
}

void A_npcController::OnSignalExchanged(int32 TargetId, A_npcCharacter* TargetChar)
{
	if (!myCharacter) return;

	F_otherIDMemory* Rec = myCharacter->mySocialMemory.Find(TargetId);
	if (!Rec) return;

	// Stato già terminale — non rielaborare (evita log duplicati e doppi effetti)
	if (Rec->state == E_otherIDState::GaveUp
		|| Rec->state == E_otherIDState::SexBlocked
		|| Rec->state == E_otherIDState::Lost) return;

	// GaveUp: inviato molto, ricevuto poco
	if (Rec->myOut_signalValue >= GAVEUP_SENT_THRESHOLD
		&& Rec->myIn_signalValue < GAVEUP_RECV_THRESHOLD)
	{
		Rec->state = E_otherIDState::GaveUp;
		myCharacter->myDesiredRank.Remove(TargetId);
		UE_LOG(LogTemp, Log, TEXT("[GAVEUP] NPC %d rinuncia a %d (out=%.1f in=%.1f)"),
			myCharacter->myId, TargetId,
			Rec->myOut_signalValue, Rec->myIn_signalValue);
		return;
	}

	// Mating trigger: segnali reciproci sopra soglia su entrambi i lati
	if (!TargetChar || !IsValid(TargetChar)) return;

	if (Rec->myOut_signalValue >= MATING_SIGNAL_THRESHOLD
		&& Rec->myIn_signalValue >= MATING_SIGNAL_THRESHOLD)
	{
		F_otherIDMemory* RecB = TargetChar->mySocialMemory.Find(myCharacter->myId);
		if (RecB
			&& RecB->myOut_signalValue >= MATING_SIGNAL_THRESHOLD
			&& RecB->myIn_signalValue  >= MATING_SIGNAL_THRESHOLD)
		{
			// Trova la zona Mating più vicina al punto medio tra i due NPC
			const FVector Midpoint = (myCharacter->GetActorLocation()
			                        + TargetChar->GetActorLocation()) * 0.5f;
			A_navMeshZone* Zone = A_navMeshZone::GetNearest(
				GetWorld(), Midpoint, E_navMeshZoneType::Mating);

			FVector MeetingPoint = Midpoint;   // fallback: si incontrano a metà
			if (Zone)
				Zone->GetRandomNavPoint(MeetingPoint);

			myCharacter->SetState(E_myState::Mating);
			TargetChar->SetState(E_myState::Mating);

			// Notifica tutti gli altri: i due sono in Mating → GaveUp nelle loro liste
			NotifyMatingStarted(myCharacter->myId, TargetId);

			// Entrambi si muovono verso lo stesso punto
			StartMatingMove(MeetingPoint);
			if (A_npcController* TargetCtrl = Cast<A_npcController>(TargetChar->GetController()))
				TargetCtrl->StartMatingMove(MeetingPoint);

			UE_LOG(LogTemp, Warning, TEXT("[MATING] NPC %d ↔ %d → punto (%.0f, %.0f, %.0f)"),
				myCharacter->myId, TargetId,
				MeetingPoint.X, MeetingPoint.Y, MeetingPoint.Z);
		}
	}
}

// ================================================================
// NotifyMatingStarted
// Quando IdA e IdB entrano in Mating, scansiona tutti gli NPC in Cruising
// e marca GaveUp chiunque avesse uno dei due nella propria myDesiredRank.
// Implementazione provvisoria — TODO: rivisitare dopo logica mating completa.
// ================================================================

void A_npcController::NotifyMatingStarted(int32 IdA, int32 IdB)
{
	if (!mySpawnManager) return;

	// Notifica overlay con le soglie vis_curr dei due NPC al momento del match
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (U_overlaySubsystem* OS = GI->GetSubsystem<U_overlaySubsystem>())
			{
				float ThreshA = 0.f, ThreshB = 0.f;
				if (A_npcCharacter** CA = mySpawnManager->NpcById.Find(IdA))
					if (*CA) ThreshA = (*CA)->myVisCurrThreshold;
				if (A_npcCharacter** CB = mySpawnManager->NpcById.Find(IdB))
					if (*CB) ThreshB = (*CB)->myVisCurrThreshold;
				OS->OnMatingStarted(IdA, IdB, ThreshA, ThreshB);
			}
		}
	}

	for (auto& Pair : mySpawnManager->NpcById)
	{
		A_npcCharacter* Npc = Pair.Value;
		if (!Npc || !IsValid(Npc)) continue;

		// Salta i due che stanno andando in Mating
		if (Npc->myId == IdA || Npc->myId == IdB) continue;

		// Solo NPC ancora in Cruising (non disturbare chi è già in Mating/Latency)
		if (Npc->myCurrentState != E_myState::Cruising) continue;

		for (int32 MatingId : { IdA, IdB })
		{
			if (!Npc->myDesiredRank.Contains(MatingId)) continue;

			// Rimuovi dal rank e aggiorna memoria
			Npc->myDesiredRank.Remove(MatingId);
			F_otherIDMemory* Rec = Npc->mySocialMemory.Find(MatingId);
			if (Rec)
				Rec->state = E_otherIDState::GaveUp;

			UE_LOG(LogTemp, Verbose,
				TEXT("[MATING_NOTIFY] NPC %d: rinuncia a %d (entrato in Mating)"),
				Npc->myId, MatingId);
		}
	}
}

// ================================================================
// Mating move — entrambi gli NPC si dirigono verso lo stesso punto
// ================================================================

void A_npcController::StartMatingMove(FVector TargetPoint)
{
	if (!myCharacter) return;

	// Ferma tutto il ciclo cruising
	GetWorldTimerManager().ClearTimer(myTimer_SignalingLogic);
	GetWorldTimerManager().ClearTimer(myTimer_LostCheck);
	GetWorldTimerManager().ClearTimer(myTimer_PreRotation);
	GetWorldTimerManager().ClearTimer(myTimer_StuckCheck);
	GetWorldTimerManager().ClearTimer(myTimer_RandomWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkDuration);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);
	GetWorldTimerManager().ClearTimer(myTimer_IdleWait);
	GetWorldTimerManager().ClearTimer(myTimer_IdleOscillation);
	GetWorldTimerManager().ClearTimer(myTimer_IdleShift);
	GetWorldTimerManager().ClearTimer(myTimer_ShiftTimeout);
	GetWorldTimerManager().ClearTimer(myTimer_MatingRetry);

	myCruisingSubState  = E_cruisingSubState::MatingWait;
	myMatingDestination = TargetPoint;
	myCharacter->SetSubStateMaterial(E_cruisingSubState::MatingWait);

	if (UCharacterMovementComponent* MoveComp = myCharacter->GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement    = true;
		MoveComp->bUseControllerDesiredRotation = false;
	}

	MoveToLocation(TargetPoint, 50.f, false);
}

// ================================================================
// Memoria — Lost check e threshold adattivo
// ================================================================

void A_npcController::TickLostCheck()
{
	if (!myCharacter) return;

	const float Now = GetWorld()->GetTimeSeconds();

	for (auto& Pair : myCharacter->mySocialMemory)
	{
		F_otherIDMemory& Rec = Pair.Value;
		if (Rec.state == E_otherIDState::SexBlocked
			|| Rec.state == E_otherIDState::GaveUp
			|| Rec.state == E_otherIDState::Lost) continue;

		if (Now - Rec.myTLastSeen > LOST_TIMEOUT_SECONDS)
		{
			Rec.state = E_otherIDState::Lost;
			myCharacter->myDesiredRank.Remove(Pair.Key);
			UE_LOG(LogTemp, Verbose, TEXT("NPC %d: %d → Lost"),
				myCharacter->myId, Pair.Key);
		}
	}
}

void A_npcController::DropVisThresholdAndPromote()
{
	if (!myCharacter || !mySubsystem) return;

	const float OldThreshold = myCharacter->myVisCurrThreshold;
	myCharacter->myVisCurrThreshold = FMath::Max(
		myCharacter->myVisCurrThreshold - myVisThresholdDropStep,
		myCharacter->myVisMinThreshold);

	if (myCharacter->myVisCurrThreshold >= OldThreshold) return;

	UE_LOG(LogTemp, Verbose, TEXT("NPC %d: threshold vis %.2f → %.2f"),
		myCharacter->myId, OldThreshold, myCharacter->myVisCurrThreshold);

	for (auto& Pair : myCharacter->mySocialMemory)
	{
		F_otherIDMemory& Rec = Pair.Value;
		if (Rec.state != E_otherIDState::NotYetDesired) continue;

		const float VisScore = mySubsystem->GetVisScore(myCharacter->myId, Pair.Key);
		if (VisScore >= myCharacter->myVisCurrThreshold)
		{
			Rec.state = E_otherIDState::Desired;
			if (!myCharacter->myDesiredRank.Contains(Pair.Key)
				&& myCharacter->myDesiredRank.Num() < 20)
			{
				myCharacter->myDesiredRank.Add(Pair.Key);
				UE_LOG(LogTemp, Verbose, TEXT("NPC %d: promosso %d (vis=%.2f)"),
					myCharacter->myId, Pair.Key, VisScore);
			}
		}
	}

	SortDesiredRank();
}

void A_npcController::SortDesiredRank()
{
	if (!myCharacter || !mySubsystem) return;

	const int32 MyId = myCharacter->myId;
	myCharacter->myDesiredRank.Sort([this, MyId](const int32& A, const int32& B)
	{
		return mySubsystem->GetVisScore(MyId, A) > mySubsystem->GetVisScore(MyId, B);
	});
}

// ================================================================
// Debug
// ================================================================

void A_npcController::TickDebugDraw()
{
	if (!myCharacter) return;

	// Overlay ID NPC — tasto 2
	if (bDebugIdEnabled)
	{
		FString IdLabel = FString::FromInt(myCharacter->myId);
		if (myCruisingSubState == E_cruisingSubState::TargetWalk && myTargetWalkTargetId != -1)
			IdLabel += FString::Printf(TEXT(" → %d"), myTargetWalkTargetId);

		const FVector IdBase = myCharacter->GetActorLocation() + FVector(0.f, 0.f, 140.f);
		DrawDebugString(GetWorld(), IdBase, IdLabel, nullptr, FColor::White, 0.25f, true, 1.2f);
	}

	// Linea verso destinazione TargetWalk — sempre visibile (non gated)
	if (myCruisingSubState == E_cruisingSubState::TargetWalk
		&& myCharacter->myCurrentState == E_myState::Cruising
		&& myPendingDestination != FVector::ZeroVector)
	{
		const float CapsuleTop = myCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		DrawDebugLine(GetWorld(),
			myCharacter->GetActorLocation() + FVector(0.f, 0.f, CapsuleTop),
			myPendingDestination             + FVector(0.f, 0.f, CapsuleTop),
			FColor::Cyan, false, 0.15f, 0, 3.f);
	}

	if (!bDebugDrawEnabled) return;
	if (myCharacter->myCurrentState != E_myState::Cruising) return;

	FString Label;
	FColor  Color;

	switch (myCruisingSubState)
	{
	case E_cruisingSubState::RandomWalk:
		Label = TEXT("WALK");      Color = FColor::Yellow;           break;
	case E_cruisingSubState::IdleWait:
		if (b_myIdleArrived) { Label = TEXT("IDLE_OK"); Color = FColor::Green; }
		else                 { Label = TEXT("IDLE>");   Color = FColor(255, 140, 0); }
		break;
	case E_cruisingSubState::MatingWait:
		if (b_myIdleArrived) { Label = TEXT("MATING_OK"); Color = FColor::Red; }
		else                 { Label = TEXT("MATING>");   Color = FColor(255, 80, 80); }
		break;
	case E_cruisingSubState::TargetWalk:
		Label = TEXT("TARGET");    Color = FColor::Cyan;              break;
	default: return;
	}

	const FVector Base = myCharacter->GetActorLocation() + FVector(0.f, 0.f, 110.f);
	DrawDebugString(GetWorld(), Base, Label, nullptr, Color, 0.25f, true, 1.5f);

	if (b_myIsShifting)
		DrawDebugString(GetWorld(), Base + FVector(0.f, 0.f, 30.f),
			TEXT("SHIFT"), nullptr, FColor(255, 0, 255), 0.25f, true, 1.5f);

	if (b_myIsStuck)
		DrawDebugString(GetWorld(), Base + FVector(0.f, 0.f, 60.f),
			TEXT("STUCK"), nullptr, FColor::Red, 0.25f, true, 1.5f);
}
