#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_npcCharacter.h"
#include "A_spawnManager.generated.h"

UCLASS()
class MACHINEDESIRING_API A_spawnManager : public AActor
{
	GENERATED_BODY()

public:

	A_spawnManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ── Parametri spawn ──────────────────────────────────────────

	// Quanti NPC spawnare (default 20 — subset della popolazione di 700)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	int32 SpawnCount = 20;

	// Raggio entro cui distribuire gli NPC (cm — 1000cm = 10m)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	float SpawnRadius = 3000.f;

	// Blueprint da spawnare — assegna BP_NPC_Character nel Details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	TSubclassOf<class A_npcCharacter> NPCClass;

	// ── Lista NPC attivi ─────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="Spawn")
	TArray<A_npcCharacter*> ActiveNPCs;

	// Lookup O(1) per ID — popolato in SpawnNPCs(), usato da A_npcController
	// Nessun UPROPERTY: int32 non è UObject
	TMap<int32, A_npcCharacter*> NpcById;

private:

	void SpawnNPCs();
	void RunStartupDebug(class U_populationSubsystem* Subsystem);
	void UpdateDebugHUD();

	float DebugHUDTimer = 0.f;
	static constexpr float HUD_UPDATE_INTERVAL = 2.f;
};