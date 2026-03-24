#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "A_playerController.generated.h"

// ================================================================
// A_playerController
// PlayerController base del progetto.
// Gestisce shortcut di debug — attualmente: "1" toggle overlay NPC.
// Da impostare come PlayerControllerClass nel GameMode.
// ================================================================

UCLASS()
class MACHINEDESIRING_API A_playerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void PlayerTick(float DeltaTime) override;

private:
	void ToggleNpcDebugDraw();
	void ToggleNpcDebugId();
};
