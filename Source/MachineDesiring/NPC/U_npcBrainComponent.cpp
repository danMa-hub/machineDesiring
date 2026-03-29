#include "U_npcBrainComponent.h"
#include "U_populationSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"

U_npcBrainComponent::U_npcBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ================================================================
// InitWithId
// Chiamato da A_spawnManager subito dopo lo spawn.
// ================================================================

void U_npcBrainComponent::InitWithId(int32 Id)
{
	myId = Id;

	UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld());
	U_populationSubsystem* Sub = GI ? GI->GetSubsystem<U_populationSubsystem>() : nullptr;
	if (!Sub || !Sub->GetPopulationSize())
	{
		UE_LOG(LogTemp, Error,
			TEXT("U_npcBrainComponent %d: U_populationSubsystem non trovato o vuoto"), myId);
		return;
	}

	InitThresholds();

	// Velocità individuale [70,100] cm/s — seed deterministico per-NPC
	{
		FRandomStream SpeedRNG(myId + 10000);
		myMaxWalkSpeed = SpeedRNG.FRandRange(70.f, 100.f);
	}

	// Estrai parametri Mutable dalla F_npcProfile
	ExtractMutableParams(Sub->GetProfile(myId));

	UE_LOG(LogTemp, Verbose,
		TEXT("U_npcBrainComponent %d: init OK — visMin=%.2f sexMin=%.2f speed=%.1f"),
		myId, myVisMinThreshold, mySexMinThreshold, myMaxWalkSpeed);
}

// ================================================================
// SetState — transizione FSM
// ================================================================

void U_npcBrainComponent::SetState(E_myState NewState)
{
	if (myCurrentState == NewState) return;

	UE_LOG(LogTemp, Verbose,
		TEXT("NPC %d: stato %d → %d"),
		myId,
		static_cast<int32>(myCurrentState),
		static_cast<int32>(NewState));

	myCurrentState = NewState;
	BP_OnStateChanged(NewState);
}

// ================================================================
// SetSubStateMaterial — debug visivo sub-stato
// ================================================================

void U_npcBrainComponent::SetSubStateMaterial(E_cruisingSubState SubState)
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

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<USkeletalMeshComponent*> Meshes;
	Owner->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* SM : Meshes)
	{
		if (!SM) continue;
		for (int32 i = 0; i < SM->GetNumMaterials(); ++i)
			SM->SetMaterial(i, Mat);
	}
}

// ================================================================
// InitThresholds
// Soglie per-NPC con varianza gaussiana deterministica (seed = myId).
//   VisStart = 0.35 + gauss(0.08, 0.03) clamp [0.03, 0.25]
//   VisMin   = 0.35 - gauss(0.12, 0.04) clamp [0.05, 0.25]
//   SexStart = 0.50 + gauss(0.08, 0.03)
//   SexMin   = 0.50 - gauss(0.12, 0.04)
// ================================================================

static float Brain_Gauss(FRandomStream& R, float Mean, float SD)
{
	const float U1 = FMath::Max(1e-6f, R.FRand());
	const float U2 = R.FRand();
	return Mean + SD * FMath::Sqrt(-2.f * FMath::Loge(U1)) * FMath::Cos(2.f * PI * U2);
}

void U_npcBrainComponent::InitThresholds()
{
	// ── Vis ──────────────────────────────────────────────────
	{
		FRandomStream R(myId);
		const float Offset = FMath::Clamp(Brain_Gauss(R, 0.08f, 0.03f), 0.03f, 0.25f);
		myVisStartThreshold = U_populationSubsystem::VIS_STANDARD_THRESHOLD + Offset;
		myVisCurrThreshold  = myVisStartThreshold;
	}
	{
		FRandomStream R(myId + 500);
		const float Drop = FMath::Clamp(Brain_Gauss(R, 0.12f, 0.04f), 0.05f, 0.25f);
		myVisMinThreshold = U_populationSubsystem::VIS_STANDARD_THRESHOLD - Drop;
	}

	// ── Sex ──────────────────────────────────────────────────
	{
		FRandomStream R(myId + 1000);
		const float Offset = FMath::Clamp(Brain_Gauss(R, 0.08f, 0.03f), 0.03f, 0.25f);
		mySexStartThreshold = U_populationSubsystem::SEX_STANDARD_THRESHOLD + Offset;
		mySexCurrThreshold  = mySexStartThreshold;
	}
	{
		FRandomStream R(myId + 1500);
		const float Drop = FMath::Clamp(Brain_Gauss(R, 0.12f, 0.04f), 0.05f, 0.25f);
		mySexMinThreshold = U_populationSubsystem::SEX_STANDARD_THRESHOLD - Drop;
	}
}

// ================================================================
// ExtractMutableParams
// Copia i valori body rilevanti da F_npcProfile → campi Mutable*.
// Il Blueprint li userà per configurare UCustomizableSkeletalComponent
// una volta che il plugin Mutable è installato.
// ================================================================

void U_npcBrainComponent::ExtractMutableParams(const F_npcProfile& Profile)
{
	MutableMuscle      = Profile.MyMuscle;
	MutableSlim        = Profile.MySlim;
	MutableHeight      = Profile.myHeightNorm;   // già normalizzato [0,1]
	MutableAge         = Profile.myAgeNorm;       // già normalizzato [0,1]
	MutableBodyDisplay = Profile.MyBodyDisplay;
	MutableHair        = Profile.MyHair;
	MutableEthnicity   = Profile.MyEthnicity;
}
