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

	// Il SkeletalMesh non deve bloccare il sight trace dell'AI Perception (ECC_Visibility).
	// Va impostato qui (BeginPlay) e non nel costruttore: il Blueprint CDO applica le sue
	// impostazioni collision (es. "CharacterMesh" di Manny) dopo il costruttore C++,
	// sovrascrivendole. BeginPlay gira sull'istanza live, dopo tutta l'inizializzazione BP.
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
		SkelMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
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
// Genera soglie per-NPC con varianza gaussiana deterministica (seed = myId).
//
// Invariante: myXxxMinThreshold < STANDARD(0.35) < myXxxStartThreshold
//   Start = 0.35 + gauss(0.08, 0.03) clamp [0.03, 0.25]  → range [0.38, 0.60]
//   Min   = 0.35 - gauss(0.12, 0.04) clamp [0.05, 0.25]  → range [0.10, 0.30]
//
// Seed offset: +500 per VisMin, +1000 per SexStart, +1500 per SexMin
// evita correlazione tra le quattro estrazioni per-NPC.
// ================================================================

static float InitThresholds_Gauss(FRandomStream& R, float Mean, float SD)
{
	const float U1 = FMath::Max(1e-6f, R.FRand());
	const float U2 = R.FRand();
	return Mean + SD * FMath::Sqrt(-2.f * FMath::Loge(U1)) * FMath::Cos(2.f * PI * U2);
}

void A_npcCharacter::InitThresholds()
{
	// ── Vis ──────────────────────────────────────────────────────
	{
		FRandomStream R(myId);
		const float Offset = FMath::Clamp(InitThresholds_Gauss(R, 0.08f, 0.03f), 0.03f, 0.25f);
		myVisStartThreshold = U_populationSubsystem::VIS_STANDARD_THRESHOLD + Offset;
		myVisCurrThreshold  = myVisStartThreshold;
	}
	{
		FRandomStream R(myId + 500);
		const float Drop = FMath::Clamp(InitThresholds_Gauss(R, 0.12f, 0.04f), 0.05f, 0.25f);
		myVisMinThreshold = U_populationSubsystem::VIS_STANDARD_THRESHOLD - Drop;
	}

	// ── Sex ──────────────────────────────────────────────────────
	{
		FRandomStream R(myId + 1000);
		const float Offset = FMath::Clamp(InitThresholds_Gauss(R, 0.08f, 0.03f), 0.03f, 0.25f);
		mySexStartThreshold = U_populationSubsystem::SEX_STANDARD_THRESHOLD + Offset;
		mySexCurrThreshold  = mySexStartThreshold;
	}
	{
		FRandomStream R(myId + 1500);
		const float Drop = FMath::Clamp(InitThresholds_Gauss(R, 0.12f, 0.04f), 0.05f, 0.25f);
		mySexMinThreshold = U_populationSubsystem::SEX_STANDARD_THRESHOLD - Drop;
	}
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
