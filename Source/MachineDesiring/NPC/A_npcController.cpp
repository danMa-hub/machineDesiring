#include "A_npcController.h"
#include "A_spawnManager.h"
#include "A_navMeshZone.h"
#include "U_overlaySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

// ================================================================
// Costanti
// ================================================================

// ── Segnalazione ────────────────────────────────────────────────
static constexpr float SIGNAL_ACTIVE_DURATION   = 1.5f;
static constexpr float SIGNAL_VALUES[]          = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f };
static constexpr float SIGNAL_VALUE_FALLBACK    = 0.1f;
static constexpr float GAVEUP_SENT_THRESHOLD    = 5.0f;
static constexpr float GAVEUP_RECV_THRESHOLD    = 1.0f;
static constexpr float MATING_SIGNAL_THRESHOLD  = 5.0f;
static constexpr float LOST_TIMEOUT_SECONDS     = 600.0f;
static constexpr float SIGNALING_TIMER_INTERVAL = 2.5f;

// ── RandomWalk ──────────────────────────────────────────────────
static constexpr float WALK_RADIUS_MIN   = 500.f;
static constexpr float WALK_RADIUS_MAX   = 2000.f;
// Durata fase RandomWalk gestita dal Blueprint (chiama OnBPPhaseDone al termine)
// Riferimento: [30, 90]s

// ── TargetWalk ──────────────────────────────────────────────────
static constexpr float TARGETWALK_RADIUS         = 150.f;
static constexpr float TARGETWALK_FORWARD_OFFSET = 300.f;
static constexpr float TARGETWALK_REACH_DIST     = 150.f;
static constexpr float TARGETWALK_SPEED_MULT     = 1.8f;
// Durata fase TargetWalk gestita dal Blueprint (chiama OnBPPhaseDone al termine)
// Riferimento: [50, 120]s

// ── Idle ────────────────────────────────────────────────────────
static constexpr float IDLE_DURATION_MIN  = 10.f;
static constexpr float IDLE_DURATION_MAX  = 55.f;
static constexpr float IDLE_YAW_OFFSET    = 40.f;

bool A_npcController::bDebugDrawEnabled = false;
bool A_npcController::bDebugIdEnabled   = false;

// ================================================================
// Costruttore
// ================================================================

A_npcController::A_npcController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(
		TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = false;
}

// ================================================================
// OnPossess / OnUnPossess
// ================================================================

void A_npcController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Crowd separation — utile anche se il movimento è in Blueprint (collision avoidance)
	if (UCrowdFollowingComponent* CrowdComp =
		Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdComp->SetCrowdSeparation(true, true);
		CrowdComp->SetCrowdSeparationWeight(1.0f, true);
		CrowdComp->SetCrowdCollisionQueryRange(200.f, true);
		CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Medium, true);
	}

	myPawn  = InPawn;
	myBrain = InPawn ? InPawn->FindComponentByClass<U_npcBrainComponent>() : nullptr;

	if (!myBrain)
	{
		UE_LOG(LogTemp, Error,
			TEXT("A_npcController: OnPossess — pawn non ha U_npcBrainComponent"));
		return;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		mySubsystem = GI->GetSubsystem<U_populationSubsystem>();
	if (!mySubsystem)
	{
		UE_LOG(LogTemp, Error,
			TEXT("A_npcController %d: U_populationSubsystem non trovato"), myBrain->myId);
		return;
	}

	for (TActorIterator<A_spawnManager> It(GetWorld()); It; ++It)
	{
		mySpawnManager = *It;
		break;
	}

	// Timer persistenti
	GetWorldTimerManager().SetTimer(myTimer_SignalingLogic, this,
		&A_npcController::TickSignalingLogic, SIGNALING_TIMER_INTERVAL, true);
	GetWorldTimerManager().SetTimer(myTimer_LostCheck, this,
		&A_npcController::TickLostCheck, 300.f, true);
	GetWorldTimerManager().SetTimer(myTimer_DebugDraw, this,
		&A_npcController::TickDebugDraw, 0.2f, true);

	// Startup ritardato — tutte le zone hanno completato BeginPlay
	GetWorldTimerManager().SetTimer(myTimer_InitialMovement, this,
		&A_npcController::StartInitialMovement, 0.2f, false);
}

void A_npcController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(myTimer_InitialMovement);
	GetWorldTimerManager().ClearTimer(myTimer_SignalingLogic);
	GetWorldTimerManager().ClearTimer(myTimer_LostCheck);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);
	GetWorldTimerManager().ClearTimer(myTimer_MatingRetry);
	GetWorldTimerManager().ClearTimer(myTimer_DebugDraw);

	myBrain        = nullptr;
	myPawn         = nullptr;
	mySubsystem    = nullptr;
	mySpawnManager = nullptr;

	Super::OnUnPossess();
}

// ================================================================
// Percezione
// ================================================================

void A_npcController::OnNearbyNPCsUpdated(const TArray<int32>& UpdatedIds)
{
	if (!myBrain || !mySubsystem) return;

	for (int32 Id : UpdatedIds)
	{
		myNearbyNPCs.AddUnique(Id);
		ProcessPerceivedNpc(Id);
	}

	SortDesiredRank();
}

void A_npcController::OnNpcLostFromSight(int32 LostId)
{
	if (!myBrain) return;

	myNearbyNPCs.Remove(LostId);

	if (F_otherIDMemory* Rec = myBrain->mySocialMemory.Find(LostId))
		Rec->myTLastSeen = GetWorld()->GetTimeSeconds();
}

void A_npcController::ProcessPerceivedNpc(int32 TargetId)
{
	if (!myBrain || !mySubsystem) return;
	if (TargetId == myBrain->myId) return;

	if (myBrain->myStaticVisBlacklist.Contains(TargetId)) return;

	const float Now = GetWorld()->GetTimeSeconds();

	if (F_otherIDMemory* Rec = myBrain->mySocialMemory.Find(TargetId))
	{
		if (Rec->state == E_otherIDState::SexBlocked) return;

		Rec->myTLastSeen = Now;

		if (Rec->state == E_otherIDState::Lost)
		{
			const float VisScore = mySubsystem->GetVisScore(myBrain->myId, TargetId);
			Rec->state = (VisScore >= myBrain->myVisCurrThreshold)
			             ? E_otherIDState::Desired : E_otherIDState::NotYetDesired;

			if (Rec->state == E_otherIDState::Desired
				&& !myBrain->myDesiredRank.Contains(TargetId)
				&& myBrain->myDesiredRank.Num() < 20)
			{
				myBrain->myDesiredRank.Add(TargetId);
			}
		}
		return;
	}

	const float VisScore = mySubsystem->GetVisScore(myBrain->myId, TargetId);

	if (VisScore < myBrain->myVisMinThreshold)
	{
		myBrain->myStaticVisBlacklist.Add(TargetId);
		return;
	}

	F_otherIDMemory NewRec;
	NewRec.targetId    = TargetId;
	NewRec.myTLastSeen = Now;
	NewRec.state = (VisScore >= myBrain->myVisCurrThreshold)
	               ? E_otherIDState::Desired : E_otherIDState::NotYetDesired;

	myBrain->mySocialMemory.Add(TargetId, NewRec);

	if (NewRec.state == E_otherIDState::Desired && myBrain->myDesiredRank.Num() < 20)
		myBrain->myDesiredRank.Add(TargetId);
}

// ================================================================
// Movimento — startup e scelta substate
// Il C++ calcola la destinazione NavMesh e la passa al Blueprint.
// Il Blueprint esegue il movimento fisico (CharacterMover / navmesh).
// Il Blueprint chiama OnBPPhaseDone() al termine di ogni fase.
// ================================================================

void A_npcController::StartInitialMovement()
{
	if (!myBrain) return;
	const int32 Id = myBrain->myId;
	if      (Id % 20 == 0) StartMatingWait();
	else if (Id % 2  == 0) StartIdleWait();
	else                    StartRandomWalk();
}

void A_npcController::StartRandomWalk()
{
	if (!myBrain || !myPawn) return;
	if (myBrain->myCurrentState != E_myState::Cruising) return;

	myCruisingSubState = E_cruisingSubState::RandomWalk;
	myBrain->SetSubStateMaterial(E_cruisingSubState::RandomWalk);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);
	BP_OnCruisingSubStateChanged(E_cruisingSubState::RandomWalk);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return;

	const FVector Origin     = myPawn->GetActorLocation();
	const float   Angle      = FMath::RandRange(0.f, 2.f * UE_PI);
	const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	const float   Dist       = FMath::Sqrt(FMath::RandRange(
		WALK_RADIUS_MIN * WALK_RADIUS_MIN,
		WALK_RADIUS_MAX * WALK_RADIUS_MAX));

	FNavLocation NavLocation;
	if (!NavSys->ProjectPointToNavigation(
		Origin + Dir * Dist, NavLocation,
		FVector(WALK_RADIUS_MAX, WALK_RADIUS_MAX, WALK_RADIUS_MAX)))
	{
		if (!NavSys->GetRandomReachablePointInRadius(Origin, WALK_RADIUS_MAX, NavLocation))
			return;
	}

	BP_ExecuteRandomWalk(NavLocation.Location);
}

void A_npcController::StartIdleWait()
{
	StartZoneWait(E_navMeshZoneType::Idle, E_cruisingSubState::IdleWait);
}

void A_npcController::StartMatingWait()
{
	StartZoneWait(E_navMeshZoneType::Mating, E_cruisingSubState::MatingWait);
}

void A_npcController::StartZoneWait(E_navMeshZoneType ZoneType, E_cruisingSubState SubState)
{
	if (!myBrain || !myPawn) return;
	if (myBrain->myCurrentState != E_myState::Cruising) return;

	myCruisingSubState = SubState;
	myBrain->SetSubStateMaterial(SubState);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);
	BP_OnCruisingSubStateChanged(SubState);

	A_navMeshZone* Zone = A_navMeshZone::GetNearest(
		GetWorld(), myPawn->GetActorLocation(), ZoneType);

	if (!Zone)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPC %d: nessuna zona %s → fallback RandomWalk"),
			myBrain->myId, (ZoneType == E_navMeshZoneType::Idle) ? TEXT("Idle") : TEXT("Mating"));
		StartRandomWalk();
		return;
	}

	FVector TargetPoint;
	if (!Zone->GetRandomNavPoint(TargetPoint))
	{
		StartRandomWalk();
		return;
	}

	myIdleBaseYaw     = Zone->arrowComp->GetComponentRotation().Yaw
	                  + FMath::RandRange(-IDLE_YAW_OFFSET, IDLE_YAW_OFFSET);
	myIdlePhaseOffset = FMath::RandRange(0.f, 2.f * UE_PI);

	const float Duration = FMath::RandRange(IDLE_DURATION_MIN, IDLE_DURATION_MAX);

	if (SubState == E_cruisingSubState::IdleWait)
		BP_ExecuteIdleWait(TargetPoint, Duration, myIdleBaseYaw, myIdlePhaseOffset);
	else
		BP_ExecuteMatingWait(TargetPoint, Duration, myIdleBaseYaw, myIdlePhaseOffset);
}

void A_npcController::StartTargetWalk()
{
	if (!myBrain || !mySpawnManager) return;
	if (myBrain->myCurrentState != E_myState::Cruising) return;

	myCruisingSubState = E_cruisingSubState::TargetWalk;
	myBrain->SetSubStateMaterial(E_cruisingSubState::TargetWalk);
	BP_OnCruisingSubStateChanged(E_cruisingSubState::TargetWalk);

	if (!GetWorldTimerManager().IsTimerActive(myTimer_TargetWalkTick))
	{
		GetWorldTimerManager().SetTimer(myTimer_TargetWalkTick, this,
			&A_npcController::TickTargetWalk, 2.0f, true);
	}

	if (myBrain->myDesiredRank.Num() == 0) return;

	// Scegli o mantieni il target
	if (myTargetWalkTargetId != -1)
	{
		U_npcBrainComponent* PrevBrain = mySpawnManager->NpcById.FindRef(myTargetWalkTargetId);
		if (!PrevBrain
			|| !IsValid(Cast<AActor>(PrevBrain->GetOwner()))
			|| PrevBrain->myCurrentState != E_myState::Cruising)
		{
			myTargetWalkTargetId = -1;
		}
		else if (myLastKnownTargetPos != FVector::ZeroVector)
		{
			APawn* TargetPawn = Cast<APawn>(PrevBrain->GetOwner());
			if (TargetPawn && FVector::Dist2D(myPawn->GetActorLocation(),
				TargetPawn->GetActorLocation()) <= TARGETWALK_REACH_DIST)
			{
				myTargetWalkTargetId = -1;
			}
		}
	}

	if (myTargetWalkTargetId == -1)
	{
		const int32 MaxIdx   = FMath::Min(myBrain->myDesiredRank.Num(), 5) - 1;
		const int32 StartIdx = FMath::RandRange(0, MaxIdx);
		for (int32 i = 0; i <= MaxIdx; ++i)
		{
			const int32 CandidateId = myBrain->myDesiredRank[(StartIdx + i) % (MaxIdx + 1)];
			U_npcBrainComponent* CandBrain = mySpawnManager->NpcById.FindRef(CandidateId);
			if (CandBrain && CandBrain->myCurrentState == E_myState::Cruising)
			{
				myTargetWalkTargetId = CandidateId;
				break;
			}
		}
		if (myTargetWalkTargetId == -1) return;
	}

	TickTargetWalk();
}

void A_npcController::TickTargetWalk()
{
	if (!myBrain || !mySpawnManager) return;
	if (myBrain->myCurrentState != E_myState::Cruising) return;
	if (myCruisingSubState != E_cruisingSubState::TargetWalk) return;
	if (myTargetWalkTargetId == -1) return;

	U_npcBrainComponent* TargetBrain = mySpawnManager->NpcById.FindRef(myTargetWalkTargetId);
	if (!TargetBrain)
	{
		myTargetWalkTargetId = -1;
		return;
	}

	if (TargetBrain->myCurrentState != E_myState::Cruising)
	{
		myTargetWalkTargetId = -1;
		return;
	}

	APawn* TargetPawn = Cast<APawn>(TargetBrain->GetOwner());
	if (!TargetPawn) { myTargetWalkTargetId = -1; return; }

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return;

	const FVector CurrentTargetPos = TargetPawn->GetActorLocation();
	const FVector ChaseOrigin      = CurrentTargetPos
	    + TargetPawn->GetActorForwardVector() * TARGETWALK_FORWARD_OFFSET;

	FNavLocation NavLocation;
	if (NavSys->GetRandomReachablePointInRadius(ChaseOrigin, TARGETWALK_RADIUS, NavLocation))
	{
		myLastKnownTargetPos = CurrentTargetPos;
		BP_ExecuteTargetWalk(NavLocation.Location,
			myBrain->myMaxWalkSpeed * TARGETWALK_SPEED_MULT,
			myTargetWalkTargetId);
		return;
	}

	// Fallback: ultima posizione nota
	if (myLastKnownTargetPos != FVector::ZeroVector)
	{
		const FVector FallbackOrigin = myLastKnownTargetPos
		    + myPawn->GetActorForwardVector() * TARGETWALK_FORWARD_OFFSET;
		if (NavSys->GetRandomReachablePointInRadius(FallbackOrigin, TARGETWALK_RADIUS, NavLocation))
		{
			BP_ExecuteTargetWalk(NavLocation.Location,
				myBrain->myMaxWalkSpeed * TARGETWALK_SPEED_MULT,
				myTargetWalkTargetId);
		}
	}
}

void A_npcController::ChooseAndStartNextCruisingSubState()
{
	if (!myBrain) return;

	myTargetWalkTargetId = -1;
	const int32 Roll = FMath::RandRange(0, 2);

	if (Roll == 0)
	{
		StartRandomWalk();
	}
	else if (Roll == 1)
	{
		StartIdleWait();
	}
	else
	{
		// TargetWalk: il Blueprint gestisce la durata (50-120s) e chiama OnBPPhaseDone
		if (myBrain->myDesiredRank.Num() > 0)
			StartTargetWalk();
		else
			StartRandomWalk();
	}
}

// ================================================================
// Callbacks Blueprint → C++
// ================================================================

void A_npcController::OnBPPhaseDone()
{
	if (!myBrain) return;
	if (myBrain->myCurrentState != E_myState::Cruising) return;
	ChooseAndStartNextCruisingSubState();
}

void A_npcController::OnBPMatingArrived()
{
	// Arrivati al punto mating — placeholder per U_matingLogic
	UE_LOG(LogTemp, Log, TEXT("[MATING] NPC %d: arrivato al punto mating"),
		myBrain ? myBrain->myId : -1);
}

void A_npcController::OnBPMatingFailed()
{
	if (!myBrain) return;
	if (myBrain->myCurrentState != E_myState::Mating) return;
	if (myMatingDestination == FVector::ZeroVector) return;

	// Retry dopo 2s
	GetWorldTimerManager().SetTimer(myTimer_MatingRetry,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (myBrain && myBrain->myCurrentState == E_myState::Mating
				&& myMatingDestination != FVector::ZeroVector)
			{
				UE_LOG(LogTemp, Log, TEXT("[MATING RETRY] NPC %d"), myBrain->myId);
				BP_ExecuteMatingMove(myMatingDestination);
			}
		}), 2.0f, false);
}

// ================================================================
// StartMatingMove
// ================================================================

void A_npcController::StartMatingMove(FVector TargetPoint)
{
	if (!myBrain) return;

	GetWorldTimerManager().ClearTimer(myTimer_SignalingLogic);
	GetWorldTimerManager().ClearTimer(myTimer_LostCheck);
	GetWorldTimerManager().ClearTimer(myTimer_TargetWalkTick);
	GetWorldTimerManager().ClearTimer(myTimer_MatingRetry);

	myCruisingSubState  = E_cruisingSubState::MatingWait;
	myMatingDestination = TargetPoint;
	myBrain->SetSubStateMaterial(E_cruisingSubState::MatingWait);
	BP_OnCruisingSubStateChanged(E_cruisingSubState::MatingWait);

	BP_ExecuteMatingMove(TargetPoint);
}

// ================================================================
// Segnalazione
// ================================================================

void A_npcController::TickSignalingLogic()
{
	if (!myBrain || !mySubsystem) return;
	if (myBrain->b_myIsSignaling) return;
	if (myBrain->myCurrentState != E_myState::Cruising) return;

	const int32 MaxScan = FMath::Min(myBrain->myDesiredRank.Num(), 5);
	for (int32 i = 0; i < MaxScan; i++)
	{
		const int32 TargetId = myBrain->myDesiredRank[i];

		F_otherIDMemory* Rec = myBrain->mySocialMemory.Find(TargetId);
		if (!Rec) continue;
		if (Rec->state == E_otherIDState::SexBlocked
			|| Rec->state == E_otherIDState::GaveUp
			|| Rec->state == E_otherIDState::Lost) continue;

		if (!mySpawnManager) continue;
		U_npcBrainComponent* TargetBrain = mySpawnManager->NpcById.FindRef(TargetId);
		if (!TargetBrain) continue;
		if (TargetBrain->b_myIsSignaling) continue;
		if (TargetBrain->myCurrentState != E_myState::Cruising) continue;

		APawn* TargetPawn = Cast<APawn>(TargetBrain->GetOwner());
		if (!TargetPawn) continue;

		// Check distanza ≤ 400cm
		if (FVector::DistSquared(myPawn->GetActorLocation(),
			TargetPawn->GetActorLocation()) > 400.f * 400.f) continue;

		// Check FOV reciproco — cono ~240° per NPC
		const FVector DirAtoB = (TargetPawn->GetActorLocation()
		                       - myPawn->GetActorLocation()).GetSafeNormal();
		if (FVector::DotProduct(myPawn->GetActorForwardVector(),   DirAtoB) < -0.2f) continue;
		if (FVector::DotProduct(TargetPawn->GetActorForwardVector(), -DirAtoB) < -0.2f) continue;

		if (TrySignaling(TargetId, TargetBrain)) break;
	}

	// Threshold accumulator
	if (myBrain->myDesiredRank.Num() == 0)
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

bool A_npcController::TrySignaling(int32 TargetId, U_npcBrainComponent* TargetBrain)
{
	if (!myBrain || !TargetBrain) return false;
	if (myBrain->b_myIsSignaling || TargetBrain->b_myIsSignaling) return false;

	// Lock atomico su entrambi
	myBrain->b_myIsSignaling    = true;
	myBrain->mySignalTarget     = TargetBrain;
	TargetBrain->b_myIsSignaling = true;
	TargetBrain->mySignalTarget  = myBrain;

	ExchangeSignals(TargetId);

	FTimerHandle ReleaseTimer;
	GetWorldTimerManager().SetTimer(ReleaseTimer,
		FTimerDelegate::CreateUObject(this, &A_npcController::ReleaseSignalLock),
		SIGNAL_ACTIVE_DURATION, false);

	return true;
}

void A_npcController::ExchangeSignals(int32 TargetId)
{
	if (!myBrain || !mySubsystem) return;

	U_npcBrainComponent* TargetBrain = myBrain->mySignalTarget.Get();
	if (!TargetBrain) return;

	APawn* TargetPawn = Cast<APawn>(TargetBrain->GetOwner());

	// Valori rank-based
	const int32 RankA   = myBrain->myDesiredRank.Find(TargetId);
	const float SentByA = (RankA >= 0 && RankA < 5) ? SIGNAL_VALUES[RankA] : SIGNAL_VALUE_FALLBACK;

	const int32 RankB   = TargetBrain->myDesiredRank.Find(myBrain->myId);
	float SentByB = 0.f;
	if      (RankB >= 0 && RankB < 5) SentByB = SIGNAL_VALUES[RankB];
	else if (RankB >= 5)               SentByB = SIGNAL_VALUE_FALLBACK;

	F_otherIDMemory* RecA = myBrain->mySocialMemory.Find(TargetId);
	F_otherIDMemory* RecB = TargetBrain->mySocialMemory.Find(myBrain->myId);

	if (RecA && RecA->state != E_otherIDState::GaveUp && RecA->state != E_otherIDState::SexBlocked)
	{
		RecA->myOut_signalValue += SentByA;
		RecA->myOut_signalCount++;
	}
	if (RecB && RecB->state != E_otherIDState::GaveUp && RecB->state != E_otherIDState::SexBlocked)
	{
		RecB->myOut_signalValue += SentByB;
		RecB->myOut_signalCount++;
	}

	// Sincronizzazione forzata: in = out dell'altro lato
	if (RecA && RecB)
	{
		RecA->myIn_signalValue = RecB->myOut_signalValue;
		RecB->myIn_signalValue = RecA->myOut_signalValue;
		RecA->myIn_signalCount = RecB->myOut_signalCount;
		RecB->myIn_signalCount = RecA->myOut_signalCount;
	}

	if (RecA)
		UE_LOG(LogTemp, Log,
			TEXT("[SIGNAL] NPC %d → %d | sent=%.1f (tot=%.1f) | recv=%.1f"),
			myBrain->myId, TargetId,
			SentByA, RecA->myOut_signalValue, RecA->myIn_signalValue);

	// Linea debug — solo se il debug è attivo
	if (bDebugDrawEnabled && myPawn && TargetPawn)
	{
		const FVector PosA = myPawn->GetActorLocation()   + FVector(0,0,90);
		const FVector PosB = TargetPawn->GetActorLocation() + FVector(0,0,90);
		DrawDebugLine(GetWorld(), PosA, PosB,
			(SentByB > 0.f) ? FColor::Green : FColor::Red,
			false, 1.0f, 0, SentByA * 10.f);
	}

	OnSignalExchanged(TargetId, TargetBrain);

	// Ri-cerca RecA: OnSignalExchanged() potrebbe aver settato GaveUp su A
	F_otherIDMemory* CheckRecA = myBrain->mySocialMemory.Find(TargetId);
	if (!(CheckRecA && CheckRecA->state == E_otherIDState::GaveUp))
	{
		if (TargetPawn)
		{
			if (A_npcController* TargetCtrl = Cast<A_npcController>(TargetPawn->GetController()))
				TargetCtrl->OnSignalExchanged(myBrain->myId, myBrain);
		}
	}
}

void A_npcController::ReleaseSignalLock()
{
	if (!myBrain) return;

	U_npcBrainComponent* TargetBrain = myBrain->mySignalTarget.Get();
	if (IsValid(TargetBrain)
		&& TargetBrain->myCurrentState != E_myState::Mating)
	{
		TargetBrain->b_myIsSignaling = false;
		TargetBrain->mySignalTarget  = nullptr;
	}

	if (myBrain->myCurrentState != E_myState::Mating)
	{
		myBrain->b_myIsSignaling = false;
		myBrain->mySignalTarget  = nullptr;
	}
}

void A_npcController::OnSignalExchanged(int32 TargetId, U_npcBrainComponent* TargetBrain)
{
	if (!myBrain) return;
	if (myBrain->myCurrentState == E_myState::Mating) return;  // già in mating — evita double trigger

	F_otherIDMemory* Rec = myBrain->mySocialMemory.Find(TargetId);
	if (!Rec) return;

	if (Rec->state == E_otherIDState::GaveUp
		|| Rec->state == E_otherIDState::SexBlocked
		|| Rec->state == E_otherIDState::Lost) return;

	// GaveUp
	if (Rec->myOut_signalValue >= GAVEUP_SENT_THRESHOLD
		&& Rec->myIn_signalValue < GAVEUP_RECV_THRESHOLD)
	{
		Rec->state = E_otherIDState::GaveUp;
		myBrain->myDesiredRank.Remove(TargetId);
		UE_LOG(LogTemp, Log, TEXT("[GAVEUP] NPC %d rinuncia a %d (out=%.1f in=%.1f)"),
			myBrain->myId, TargetId, Rec->myOut_signalValue, Rec->myIn_signalValue);
		return;
	}

	// Mating trigger
	if (!IsValid(TargetBrain)) return;

	if (Rec->myOut_signalValue >= MATING_SIGNAL_THRESHOLD
		&& Rec->myIn_signalValue >= MATING_SIGNAL_THRESHOLD)
	{
		F_otherIDMemory* RecB = TargetBrain->mySocialMemory.Find(myBrain->myId);
		if (RecB
			&& RecB->myOut_signalValue >= MATING_SIGNAL_THRESHOLD
			&& RecB->myIn_signalValue  >= MATING_SIGNAL_THRESHOLD)
		{
			APawn* TargetPawn = Cast<APawn>(TargetBrain->GetOwner());

			const FVector Midpoint = myPawn
				? (myPawn->GetActorLocation() + (TargetPawn ? TargetPawn->GetActorLocation() : myPawn->GetActorLocation())) * 0.5f
				: FVector::ZeroVector;

			A_navMeshZone* Zone = A_navMeshZone::GetNearest(
				GetWorld(), Midpoint, E_navMeshZoneType::Mating);

			FVector MeetingPoint = Midpoint;
			if (Zone) Zone->GetRandomNavPoint(MeetingPoint);

			myBrain->SetState(E_myState::Mating);
			TargetBrain->SetState(E_myState::Mating);

			NotifyMatingStarted(myBrain->myId, TargetId);

			StartMatingMove(MeetingPoint);
			if (TargetPawn)
			{
				if (A_npcController* TargetCtrl = Cast<A_npcController>(TargetPawn->GetController()))
					TargetCtrl->StartMatingMove(MeetingPoint);
			}

			UE_LOG(LogTemp, Warning, TEXT("[MATING] NPC %d ↔ %d → (%.0f,%.0f,%.0f)"),
				myBrain->myId, TargetId,
				MeetingPoint.X, MeetingPoint.Y, MeetingPoint.Z);
		}
	}
}

// ================================================================
// NotifyMatingStarted
// ================================================================

void A_npcController::NotifyMatingStarted(int32 IdA, int32 IdB)
{
	if (!mySpawnManager) return;

	// Notifica overlay
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (U_overlaySubsystem* OS = GI->GetSubsystem<U_overlaySubsystem>())
			{
				float ThreshA = 0.f, ThreshB = 0.f;
				if (U_npcBrainComponent** BA = mySpawnManager->NpcById.Find(IdA))
					if (*BA) ThreshA = (*BA)->myVisCurrThreshold;
				if (U_npcBrainComponent** BB = mySpawnManager->NpcById.Find(IdB))
					if (*BB) ThreshB = (*BB)->myVisCurrThreshold;
				OS->OnMatingStarted(IdA, IdB, ThreshA, ThreshB);
			}
		}
	}

	// Marca GaveUp in tutti gli NPC che avevano IdA o IdB nel rank
	for (auto& Pair : mySpawnManager->NpcById)
	{
		U_npcBrainComponent* Brain = Pair.Value;
		if (!Brain) continue;
		if (Brain->myId == IdA || Brain->myId == IdB) continue;
		if (Brain->myCurrentState != E_myState::Cruising) continue;

		for (int32 MatingId : { IdA, IdB })
		{
			if (!Brain->myDesiredRank.Contains(MatingId)) continue;
			Brain->myDesiredRank.Remove(MatingId);
			if (F_otherIDMemory* Rec = Brain->mySocialMemory.Find(MatingId))
				Rec->state = E_otherIDState::GaveUp;
		}
	}
}

// ================================================================
// Memoria — Lost check e threshold adattivo
// ================================================================

void A_npcController::TickLostCheck()
{
	if (!myBrain) return;

	const float Now = GetWorld()->GetTimeSeconds();

	for (auto& Pair : myBrain->mySocialMemory)
	{
		F_otherIDMemory& Rec = Pair.Value;
		if (Rec.state == E_otherIDState::SexBlocked
			|| Rec.state == E_otherIDState::GaveUp
			|| Rec.state == E_otherIDState::Lost) continue;

		if (Now - Rec.myTLastSeen > LOST_TIMEOUT_SECONDS)
		{
			Rec.state = E_otherIDState::Lost;
			myBrain->myDesiredRank.Remove(Pair.Key);
			UE_LOG(LogTemp, Verbose, TEXT("NPC %d: %d → Lost"), myBrain->myId, Pair.Key);
		}
	}
}

void A_npcController::DropVisThresholdAndPromote()
{
	if (!myBrain || !mySubsystem) return;

	const float OldThreshold = myBrain->myVisCurrThreshold;
	myBrain->myVisCurrThreshold = FMath::Max(
		myBrain->myVisCurrThreshold - myVisThresholdDropStep,
		myBrain->myVisMinThreshold);

	if (myBrain->myVisCurrThreshold >= OldThreshold) return;

	UE_LOG(LogTemp, Verbose, TEXT("NPC %d: threshold vis %.2f → %.2f"),
		myBrain->myId, OldThreshold, myBrain->myVisCurrThreshold);

	for (auto& Pair : myBrain->mySocialMemory)
	{
		F_otherIDMemory& Rec = Pair.Value;
		if (Rec.state != E_otherIDState::NotYetDesired) continue;

		const float VisScore = mySubsystem->GetVisScore(myBrain->myId, Pair.Key);
		if (VisScore >= myBrain->myVisCurrThreshold)
		{
			Rec.state = E_otherIDState::Desired;
			if (!myBrain->myDesiredRank.Contains(Pair.Key)
				&& myBrain->myDesiredRank.Num() < 20)
			{
				myBrain->myDesiredRank.Add(Pair.Key);
				UE_LOG(LogTemp, Verbose, TEXT("NPC %d: promosso %d (vis=%.2f)"),
					myBrain->myId, Pair.Key, VisScore);
			}
		}
	}

	SortDesiredRank();
}

void A_npcController::SortDesiredRank()
{
	if (!myBrain || !mySubsystem) return;

	const int32 MyId = myBrain->myId;
	myBrain->myDesiredRank.Sort([this, MyId](const int32& A, const int32& B)
	{
		return mySubsystem->GetVisScore(MyId, A) > mySubsystem->GetVisScore(MyId, B);
	});
}

// ================================================================
// Debug
// ================================================================

void A_npcController::TickDebugDraw()
{
	if (!bDebugDrawEnabled && !bDebugIdEnabled) return;
	if (!myBrain || !myPawn) return;

	if (bDebugIdEnabled)
	{
		FString Label = FString::FromInt(myBrain->myId);
		if (myCruisingSubState == E_cruisingSubState::TargetWalk
			&& myTargetWalkTargetId != -1)
		{
			Label += FString::Printf(TEXT(" → %d"), myTargetWalkTargetId);
		}
		DrawDebugString(GetWorld(),
			myPawn->GetActorLocation() + FVector(0, 0, 120),
			Label, nullptr, FColor::White, 0.25f);
	}

	if (bDebugDrawEnabled)
	{
		FColor StateColor = FColor::White;
		FString SubLabel  = TEXT("?");

		if (myBrain->myCurrentState == E_myState::Mating)
		{
			StateColor = FColor::Red;
			SubLabel   = TEXT("MAT");
		}
		else
		{
			switch (myCruisingSubState)
			{
			case E_cruisingSubState::RandomWalk:
				StateColor = FColor::Cyan;   SubLabel = TEXT("RW");  break;
			case E_cruisingSubState::IdleWait:
				StateColor = FColor::Green;  SubLabel = TEXT("IDL"); break;
			case E_cruisingSubState::MatingWait:
				StateColor = FColor::Purple; SubLabel = TEXT("MWA"); break;
			case E_cruisingSubState::TargetWalk:
				StateColor = FColor::Orange; SubLabel = TEXT("TW");  break;
			}
		}

		DrawDebugString(GetWorld(),
			myPawn->GetActorLocation() + FVector(0, 0, 100),
			SubLabel, nullptr, StateColor, 0.25f);
	}
}
