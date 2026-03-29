#include "A_spawnManager.h"
#include "A_npcCharacter.h"
#include "U_populationSubsystem.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "U_overlaySubsystem.h"
#include "UW_mainOverlay.h"
#include "UW_populationPanel.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

A_spawnManager::A_spawnManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void A_spawnManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	DebugHUDTimer += DeltaSeconds;
	if (DebugHUDTimer >= HUD_UPDATE_INTERVAL)
	{
		DebugHUDTimer = 0.f;
		UpdateDebugHUD();
	}
}

void A_spawnManager::BeginPlay()
{
	Super::BeginPlay();
	SpawnNPCs();
}

void A_spawnManager::SpawnNPCs()
{
	if (!NPCClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnManager: NPCClass non assegnata — assegna BP_NPC_Character nel Details panel"));
		return;
	}

	U_populationSubsystem* Subsystem = UGameplayStatics::GetGameInstance(this)
	                                     ->GetSubsystem<U_populationSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnManager: Subsystem non trovato"));
		return;
	}

	// Verifica che ci siano abbastanza profili
	const int32 AvailableProfiles = Subsystem->GetPopulationSize();
	const int32 ActualSpawnCount  = FMath::Min(SpawnCount, AvailableProfiles);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < ActualSpawnCount; i++)
	{
		// Cerca un punto raggiungibile sul NavMesh entro SpawnRadius
		FNavLocation NavLocation;
		FVector SpawnLocation = GetActorLocation();

		if (NavSys && NavSys->GetRandomReachablePointInRadius(
			GetActorLocation(), SpawnRadius, NavLocation))
		{
			SpawnLocation = NavLocation.Location;
		}
		else
		{
			// Fallback: posizione random nel raggio senza NavMesh
			const float Angle = FMath::RandRange(0.f, 360.f);
			const float Dist  = FMath::RandRange(0.f, SpawnRadius);
			SpawnLocation = GetActorLocation()
			              + FVector(FMath::Cos(Angle) * Dist,
			                        FMath::Sin(Angle) * Dist,
			                        0.f);
		}

		FRotator SpawnRotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);

		A_npcCharacter* NPC = GetWorld()->SpawnActor<A_npcCharacter>(
			NPCClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (NPC)
		{
			// Assegna id progressivo — ogni NPC prende un profilo diverso
			NPC->InitWithId(i);
			ActiveNPCs.Add(NPC);
			NpcById.Add(i, NPC);   // cache O(1) per lookup in A_npcController
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("SpawnManager: spawnati %d NPC"), ActiveNPCs.Num());

	// Analisi incrociata e sfere debug
	RunStartupDebug(Subsystem);

// 1. Raccogli gli ID degli NPC appena spawnati
	TArray<int32> SpawnedIDs;
	for (A_npcCharacter* NPC : ActiveNPCs)
	{
		if (IsValid(NPC))
		{
			SpawnedIDs.Add(NPC->myId); 
		}
	}

	// 2. Calcola le statistiche (questo è immediato e va bene qui)
	if (Subsystem)
	{
		Subsystem->CalculateSubsetStats(SpawnedIDs);
	}

	// 3. RITARDO DI 0.5 SECONDI PER L'INTERFACCIA
	// Creiamo un timer che aspetta che la UI sia pronta prima di inviare i dati
	FTimerHandle TempHandle;
	FTimerDelegate TimerDel;
	TimerDel.BindLambda([this, Subsystem, SpawnedIDs]()
	{
		if (!IsValid(this)) return; // Sicurezza extra

		U_overlaySubsystem* OverlaySub = UGameplayStatics::GetGameInstance(this)->GetSubsystem<U_overlaySubsystem>();
		if (OverlaySub && OverlaySub->MainOverlay && OverlaySub->MainOverlay->PopulationPanel)
		{
			OverlaySub->MainOverlay->PopulationPanel->RefreshData(Subsystem, SpawnedIDs);
		}
	});

	// Fa partire il timer di mezzo secondo (0.5f)
	GetWorld()->GetTimerManager().SetTimer(TempHandle, TimerDel, 0.5f, false);
}

// ================================================================
// RunStartupDebug
// ================================================================

void A_spawnManager::RunStartupDebug(U_populationSubsystem* Subsystem)
{
	if (!Subsystem || ActiveNPCs.Num() == 0) return;

	// Soglie allineate al runtime (A_npcCharacter.h)
	static constexpr float VIS_THRESHOLD = 0.05f;
	static constexpr float SEX_THRESHOLD = 0.05f;

	// ── Confronto incrociato coppie — usa matrice pre-calcolata (O(1)) ──
	int32 MutualVisCount = 0;
	int32 MutualSexCount = 0;
	float MaxVisScore    = 0.f;

	for (int32 i = 0; i < ActiveNPCs.Num(); ++i)
	{
		for (int32 j = i + 1; j < ActiveNPCs.Num(); j++)
		{
			A_npcCharacter* A = ActiveNPCs[i];
			A_npcCharacter* B = ActiveNPCs[j];
			if (!IsValid(A) || !IsValid(B)) continue;

			const float VisAB = Subsystem->GetVisScore(A->myId, B->myId);
			const float VisBA = Subsystem->GetVisScore(B->myId, A->myId);
			MaxVisScore = FMath::Max(MaxVisScore, FMath::Max(VisAB, VisBA));
			const bool  bMutualVis = VisAB >= VIS_THRESHOLD && VisBA >= VIS_THRESHOLD;

			const float SexAB = Subsystem->GetSexScore(A->myId, B->myId);
			const float SexBA = Subsystem->GetSexScore(B->myId, A->myId);
			const bool  bMutualSex = SexAB >= SEX_THRESHOLD && SexBA >= SEX_THRESHOLD;
			const bool  bFullMatch = bMutualVis && bMutualSex;

			if (bMutualVis) MutualVisCount++;
			if (bFullMatch) MutualSexCount++;
		}
	}

	// Diagnostica: max VisScore osservato → aiuta a calibrare la soglia
	UE_LOG(LogTemp, Warning,
		TEXT("StartupDebug: VisScore max = %.3f tra %d NPC (soglia %.2f)"),
		MaxVisScore, ActiveNPCs.Num(), VIS_THRESHOLD);

	// ── Riepilogo permanente ─────────────────────────────────────────
	// const FString Summary = FString::Printf(
	// 	TEXT("── %d NPC  |  mutual VIS: %d  |  mutual VIS+SEX: %d  |  maxVis:%.2f ──"),
	// 	ActiveNPCs.Num(), MutualVisCount, MutualSexCount, MaxVisScore);
	// GEngine->AddOnScreenDebugMessage(-1, TNumericLimits<float>::Max(), FColor::Green, Summary);
}

// ================================================================
// UpdateDebugHUD — stats popolazione ogni 2s sul viewport
// ================================================================

void A_spawnManager::UpdateDebugHUD()
{
	if (!GEngine || ActiveNPCs.Num() == 0) return;

	int32 NCruising = 0, NMating = 0, NLatency = 0;
	int32 NSignaling = 0, NWithDesired = 0;
	float TotalSent = 0.f, TotalReceived = 0.f;
	int32 ValidCount = 0;

	for (A_npcCharacter* NPC : ActiveNPCs)
	{
		if (!IsValid(NPC)) continue;
		ValidCount++;

		switch (NPC->myCurrentState)
		{
			case E_myState::Cruising: NCruising++; break;
			case E_myState::Mating:   NMating++;   break;
			case E_myState::Latency:  NLatency++;  break;
		}

		if (NPC->b_myIsSignaling) NSignaling++;

		if (NPC->myDesiredRank.Num() > 0)
		{
			NWithDesired++;
			for (const auto& Pair : NPC->mySocialMemory)
			{
				TotalSent     += Pair.Value.myOut_signalValue;
				TotalReceived += Pair.Value.myIn_signalValue;
			}
		}
	}

	const float AvgSent     = (ValidCount > 0) ? TotalSent     / ValidCount : 0.f;
	const float AvgReceived = (ValidCount > 0) ? TotalReceived / ValidCount : 0.f;

	// // Riga 1 — stati FSM
	// const FString States = FString::Printf(
	// 	TEXT("STATI: Cruising:%d  Mating:%d  Latency:%d  |  Signaling:%d"),
	// 	NCruising, NMating, NLatency, NSignaling);

	// // Riga 2 — segnali da mySocialMemory
	// const FString Signals = FString::Printf(
	// 	TEXT("SEGNALI (media/NPC): inviati:%.1f  ricevuti:%.1f  |  con desired:%d/%d"),
	// 	AvgSent, AvgReceived, NWithDesired, ValidCount);

	// // Stampa con chiavi fisse (sovrascrive ogni update — non si accumulano)
	// GEngine->AddOnScreenDebugMessage(10, HUD_UPDATE_INTERVAL + 0.5f, FColor::Cyan,  States);
	// GEngine->AddOnScreenDebugMessage(11, HUD_UPDATE_INTERVAL + 0.5f, FColor::White, Signals);
}