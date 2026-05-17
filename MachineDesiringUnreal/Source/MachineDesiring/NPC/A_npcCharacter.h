#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "A_npcCharacter.generated.h"

// ================================================================
// A_npcCharacter
// Shell minima per NPC basati su ACharacter (ThirdPerson standard).
// Crea U_npcBrainComponent in costruttore — porta tutta la logica NPC.
//
// NOTA ARCHITETTURALE:
//   BP_NPC_Character (GASP/Mover) eredita da MY_MOVERTEST, NON da questa
//   classe. Aggiunge U_npcBrainComponent nel Blueprint Editor.
//   Questa classe rimane come parent alternativo per NPC ThirdPerson
//   (futuro swap distanza camera).
// ================================================================

class U_npcBrainComponent;

UCLASS()
class MACHINEDESIRING_API A_npcCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	A_npcCharacter();

	// Accesso rapido al BrainComponent — cachato in costruttore
	UFUNCTION(BlueprintCallable, Category="NPC")
	U_npcBrainComponent* GetBrain() const { return BrainComponent; }

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC",
		meta=(AllowPrivateAccess="true"))
	U_npcBrainComponent* BrainComponent = nullptr;
};
