#include "A_npcCharacter.h"
#include "U_npcBrainComponent.h"

A_npcCharacter::A_npcCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Crea BrainComponent — porta tutta l'identità e la logica NPC
	BrainComponent = CreateDefaultSubobject<U_npcBrainComponent>(TEXT("NpcBrainComponent"));
}

void A_npcCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Il SkeletalMesh non deve bloccare il sight trace di AIPerception (ECC_Visibility).
	// In BeginPlay (non nel costruttore) perché il Blueprint CDO sovrascrive
	// le impostazioni collision dopo il costruttore C++.
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
		SkelMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
}
