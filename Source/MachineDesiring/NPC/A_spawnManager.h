#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "U_npcBrainComponent.h"
#include "A_spawnManager.generated.h"

// ================================================================
// A_spawnManager — post-refactoring GASP/Mover
//
// Spawna APawn generici (BP_NPC_Character eredita MY_MOVERTEST).
// Dopo lo spawn: trova U_npcBrainComponent → InitWithId → BP_ApplyMutableProfile.
// NpcById mappa id → BrainComponent per lookup O(1) nel controller.
// ================================================================

UCLASS()
class MACHINEDESIRING_API A_spawnManager : public AActor
{
	GENERATED_BODY()

public:

	A_spawnManager();

	virtual void BeginPlay() override;

	// ── Parametri spawn ──────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 SpawnCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float SpawnRadius = 3000.f;

	// Assegna BP_NPC_Character (eredita MY_MOVERTEST) nel Details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	TSubclassOf<APawn> NPCClass;

	// ── Lista NPC attivi ─────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="Spawn")
	TArray<APawn*> ActiveNPCs;

	// Lookup O(1) per ID — BrainComponent del pawn
	// UPROPERTY necessario: il value è UObject* — il GC deve tracciare questi riferimenti
	UPROPERTY()
	TMap<int32, U_npcBrainComponent*> NpcById;

private:

	void SpawnNPCs();
	void RunStartupDebug(class U_populationSubsystem* Subsystem);
	void UpdateDebugHUD();

	FTimerHandle myTimer_DebugHUD;
};
