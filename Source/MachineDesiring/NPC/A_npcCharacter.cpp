// Fill out your copyright notice in the Description page of Project Settings.

#include "A_npcCharacter.h"
#include "U_populationSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

A_npcCharacter::A_npcCharacter()
{
	PrimaryActorTick.bCanEverTick = false;   // logica in A_npcController, non nel tick

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// Velocità
		MoveComp->MaxWalkSpeed = 80.f;

		// Partenza morbida — ~1s per raggiungere velocità massima
		MoveComp->MaxAcceleration = 100.f;

		// Arrivo morbido — scivola dolcemente alla destinazione
		MoveComp->bUseSeparateBrakingFriction = true;
		MoveComp->BrakingDecelerationWalking  = 50.f;
		MoveComp->GroundFriction              = 2.0f;

		// Orientamento — ruota verso la direzione di movimento, non del controller
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate              = FRotator(0.f, myRotationRate, 0.f);

		// RVO disabilitato — avoidance gestito da UCrowdFollowingComponent
		MoveComp->bUseRVOAvoidance = false;
	}

	// Il pawn non deve ruotare con il controller (gestisce MoveComp->bOrientRotationToMovement)
	bUseControllerRotationYaw = false;
}

void A_npcCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// ================================================================
// SetSubStateMaterial
// Applica il materiale corrispondente al sub-stato su tutti gli slot.
// Chiamato da A_npcController quando cambia myCruisingSubState.
// Se il materiale non è assegnato nel Blueprint, non fa nulla.
// ================================================================

void A_npcCharacter::SetSubStateMaterial(E_cruisingSubState SubState)
{
	UMaterialInterface* Mat = nullptr;
	switch (SubState)
	{
	case E_cruisingSubState::RandomWalk:  Mat = Mat_RandomWalk;  break;
	case E_cruisingSubState::IdleWait:    Mat = Mat_Idle;         break;
	case E_cruisingSubState::MatingWait:  Mat = Mat_Mating;       break;
	case E_cruisingSubState::TargetWalk:  Mat = Mat_TargetWalk;   break;
	default: break;
	}

	if (!Mat) return;

	// Applica a tutti i componenti SkeletalMesh dell'actor —
	// necessario quando il mesh visivo è un componente figlio nel Blueprint
	// e non il componente default di ACharacter (GetMesh()).
	TArray<USkeletalMeshComponent*> Meshes;
	GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* SkelMesh : Meshes)
	{
		if (!SkelMesh) continue;
		for (int32 i = 0; i < SkelMesh->GetNumMaterials(); ++i)
			SkelMesh->SetMaterial(i, Mat);
	}
}

// ================================================================
// InitWithId
// Chiamato da A_spawnManager subito dopo lo spawn.
// Assegna myId, verifica profilo nel subsystem, inizializza soglie.
// ================================================================

void A_npcCharacter::InitWithId(int32 Id)
{
	myId = Id;

	// Verifica che il profilo esista nel subsystem
	U_populationSubsystem* Sub = UGameplayStatics::GetGameInstance(this)
	                             ->GetSubsystem<U_populationSubsystem>();
	if (!Sub || !Sub->GetPopulationSize())
	{
		UE_LOG(LogTemp, Error,
			TEXT("A_npcCharacter %d: U_populationSubsystem non trovato o vuoto"), myId);
		return;
	}

	InitThresholds();

	// Velocità individuale — varianza deterministica per NPC [70, 100] cm/s
	// Seed offset rispetto a InitThresholds (che usa myId) per evitare correlazione
	{
		FRandomStream SpeedRNG(myId + 10000);
		myMaxWalkSpeed = SpeedRNG.FRandRange(70.f, 100.f);
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = myMaxWalkSpeed;
		}
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("A_npcCharacter %d: init OK — visMin=%.2f sexMin=%.2f speed=%.1f"),
		myId, myVisMinThreshold, mySexMinThreshold, myMaxWalkSpeed);
}

// ================================================================
// InitThresholds
// Soglie di partenza uguali per tutti (Start = Curr).
// myVisMinThreshold: lieve varianza per-NPC — alcuni NPC sono
//   strutturalmente più selettivi e non abbassano oltre il loro limite.
// mySexMinThreshold: 0.40 fisso — hard limit algoritmico (§2.1).
// ================================================================

void A_npcCharacter::InitThresholds()
{
	// Vis start + curr — stessa soglia iniziale
	myVisStartThreshold = 0.35f;
	myVisCurrThreshold  = myVisStartThreshold;

	// Vis min — lieve varianza per-NPC, deterministica con seed = myId.
	// FRandomStream locale: ogni NPC produce sempre lo stesso valore
	// indipendentemente dall'ordine di spawn.
	// gauss(0.10, 0.02) clamp [0.05, 0.20]
	{
		FRandomStream LocalRNG(myId);
		const float U1 = FMath::Max(1e-6f, LocalRNG.FRand());
		const float U2 = LocalRNG.FRand();
		const float Gauss = 0.10f + 0.02f
		    * FMath::Sqrt(-2.f * FMath::Loge(U1)) * FMath::Cos(2.f * PI * U2);
		myVisMinThreshold = FMath::Clamp(Gauss, 0.05f, 0.20f);
	}

	// Sex start + curr
	mySexStartThreshold = 0.50f;
	mySexCurrThreshold  = mySexStartThreshold;

	// Sex min — fisso, hard limit algoritmico (Algoritmo_Cruising §2.1)
	mySexMinThreshold = 0.40f;
}

// ================================================================
// SetState
// Transizione FSM con log.
// La logica di transizione vive in U_cruisingLogic / U_matingLogic —
// questo è solo il setter con side effects minimi.
// ================================================================

void A_npcCharacter::SetState(E_myState NewState)
{
	if (myCurrentState == NewState) return;

	UE_LOG(LogTemp, Verbose,
		TEXT("NPC %d: %d -> %d"),
		myId,
		static_cast<int32>(myCurrentState),
		static_cast<int32>(NewState));

	myCurrentState = NewState;
}
