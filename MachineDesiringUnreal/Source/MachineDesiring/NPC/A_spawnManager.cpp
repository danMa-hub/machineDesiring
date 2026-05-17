#include "A_spawnManager.h"
#include "U_populationSubsystem.h"
#include "U_overlaySubsystem.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

A_spawnManager::A_spawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void A_spawnManager::BeginPlay()
{
	Super::BeginPlay();
	SpawnNPCs();

	// Timer UpdateDebugHUD disabilitato — decommentare quando il debug HUD sarà ripristinato
	// GetWorld()->GetTimerManager().SetTimer(myTimer_DebugHUD, this,
	//     &A_spawnManager::UpdateDebugHUD, 2.f, true);
}

// ================================================================
// SpawnNPCs
// ================================================================

void A_spawnManager::SpawnNPCs()
{
	if (!NPCClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("SpawnManager: NPCClass non assegnata — assegna BP_NPC_Character nel Details panel"));
		return;
	}

	UGameInstance* GI_Spawn = UGameplayStatics::GetGameInstance(this);
	U_populationSubsystem* Subsystem = GI_Spawn ? GI_Spawn->GetSubsystem<U_populationSubsystem>() : nullptr;
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnManager: Subsystem non trovato"));
		return;
	}

	const int32 AvailableProfiles = Subsystem->GetPopulationSize();
	const int32 ActualSpawnCount  = FMath::Min(SpawnCount, AvailableProfiles);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < ActualSpawnCount; i++)
	{
		// Punto raggiungibile sul NavMesh
		FNavLocation NavLocation;
		FVector SpawnLocation = GetActorLocation();

		if (NavSys && NavSys->GetRandomReachablePointInRadius(
			GetActorLocation(), SpawnRadius, NavLocation))
		{
			SpawnLocation = NavLocation.Location;
		}
		else
		{
			const float Angle = FMath::RandRange(0.f, 2.f * UE_PI);
			const float Dist  = FMath::RandRange(0.f, SpawnRadius);
			SpawnLocation = GetActorLocation()
			              + FVector(FMath::Cos(Angle) * Dist,
			                        FMath::Sin(Angle) * Dist, 0.f);
		}

		const FRotator SpawnRotation(0.f, FMath::RandRange(0.f, 360.f), 0.f);

		APawn* NPC = GetWorld()->SpawnActor<APawn>(
			NPCClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (!NPC) continue;

		// Cerca il BrainComponent — deve essere presente nel Blueprint
		U_npcBrainComponent* Brain = NPC->FindComponentByClass<U_npcBrainComponent>();
		if (!Brain)
		{
			UE_LOG(LogTemp, Error,
				TEXT("SpawnManager: NPC spawnato non ha U_npcBrainComponent — "
				     "aggiungilo a BP_NPC_Character nel Blueprint Editor"));
			NPC->Destroy();
			continue;
		}

		// Inizializza identità, soglie e parametri Mutable
		Brain->InitWithId(i);
		ActiveNPCs.Add(NPC);
		NpcById.Add(i, Brain);

		// Il Blueprint riceve i valori Mutable e configura UCustomizableSkeletalComponent
		Brain->BP_ApplyMutableProfile();
	}

	UE_LOG(LogTemp, Warning, TEXT("SpawnManager: spawnati %d NPC"), ActiveNPCs.Num());

	RunStartupDebug(Subsystem);

	// Calcola statistiche subset
	TArray<int32> SpawnedIDs;
	for (auto& Pair : NpcById) SpawnedIDs.Add(Pair.Key);
	Subsystem->CalculateSubsetStats(SpawnedIDs);

	// Aggiorna overlay con 0.5s di ritardo (attende che la UI sia pronta)
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
			U_overlaySubsystem* OverlaySub = GI ? GI->GetSubsystem<U_overlaySubsystem>() : nullptr;
			if (OverlaySub) OverlaySub->RefreshPopulationPanel();
		}), 0.5f, false);
}

// ================================================================
// RunStartupDebug
// ================================================================

void A_spawnManager::RunStartupDebug(U_populationSubsystem* Subsystem)
{
	if (!Subsystem || ActiveNPCs.Num() == 0) return;

	static constexpr float VIS_THRESHOLD = 0.05f;
	static constexpr float SEX_THRESHOLD = 0.05f;

	int32 MutualVisCount = 0;
	int32 MutualSexCount = 0;
	float MaxVisScore    = 0.f;

	TArray<int32> IDs;
	NpcById.GetKeys(IDs);

	for (int32 i = 0; i < IDs.Num(); ++i)
	{
		for (int32 j = i + 1; j < IDs.Num(); ++j)
		{
			const int32 IdA = IDs[i];
			const int32 IdB = IDs[j];

			const float VisAB = Subsystem->GetVisScore(IdA, IdB);
			const float VisBA = Subsystem->GetVisScore(IdB, IdA);
			MaxVisScore = FMath::Max(MaxVisScore, FMath::Max(VisAB, VisBA));

			if (VisAB >= VIS_THRESHOLD && VisBA >= VIS_THRESHOLD)
			{
				MutualVisCount++;
				const float SexAB = Subsystem->GetSexScore(IdA, IdB);
				const float SexBA = Subsystem->GetSexScore(IdB, IdA);
				if (SexAB >= SEX_THRESHOLD && SexBA >= SEX_THRESHOLD)
					MutualSexCount++;
			}
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("StartupDebug: VisScore max=%.3f | mutualVis=%d | mutualVisSex=%d"),
		MaxVisScore, MutualVisCount, MutualSexCount);
}

// ================================================================
// UpdateDebugHUD — stats ogni 2s
// ================================================================

void A_spawnManager::UpdateDebugHUD()
{
	if (!GEngine || NpcById.Num() == 0) return;

	int32 NCruising = 0, NMating = 0, NLatency = 0, NSignaling = 0, NWithDesired = 0;

	for (auto& Pair : NpcById)
	{
		U_npcBrainComponent* Brain = Pair.Value;
		if (!Brain) continue;

		switch (Brain->myCurrentState)
		{
			case E_myState::Cruising: NCruising++; break;
			case E_myState::Mating:   NMating++;   break;
			case E_myState::Latency:  NLatency++;  break;
		}
		if (Brain->b_myIsSignaling)          NSignaling++;
		if (Brain->myDesiredRank.Num() > 0)  NWithDesired++;
	}

	// Rimuovi le righe di commento per abilitare il debug HUD
	// const FString States = FString::Printf(
	// 	TEXT("STATI: Cruising:%d  Mating:%d  Latency:%d  Signaling:%d  Desired:%d"),
	// 	NCruising, NMating, NLatency, NSignaling, NWithDesired);
	// GEngine->AddOnScreenDebugMessage(10, HUD_UPDATE_INTERVAL + 0.5f, FColor::Cyan, States);
}
