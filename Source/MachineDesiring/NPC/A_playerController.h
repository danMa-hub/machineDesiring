#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "A_playerController.generated.h"

class U_overlaySubsystem;

// ================================================================
// A_playerController
// PlayerController base del progetto.
// Tasto 1: toggle debug sub-stati NPC
// Tasto 2: toggle debug ID NPC
// Tasto 3: toggle PopulationPanel overlay (statistiche)
// Da impostare come PlayerControllerClass nel GameMode.
// ================================================================

UCLASS()
class MACHINEDESIRING_API A_playerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	void TogglePopulationPanel();
	void ToggleRuntimePanel();
	void ToggleNpcDebugDraw();
	void ToggleNpcDebugId();

	U_overlaySubsystem* GetOverlaySub() const;
};
