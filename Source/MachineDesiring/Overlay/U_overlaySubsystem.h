#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UW_mainOverlay.h"
#include "U_overlaySubsystem.generated.h"

class U_populationSubsystem;
class A_spawnManager;
class A_npcCharacter;

// ================================================================
// U_overlaySubsystem
// Aggregatore dati overlay. UGameInstanceSubsystem — si inizializza
// automaticamente con il GameInstance.
//
// SetupOverlay() va chiamato da BeginPlay (GameMode o PlayerController)
// dopo che il mondo e il PlayerController esistono.
//
// Timer 3s → TickOverlayData() per View 2 (runtime population).
// OnSignalExchanged() → entry point da A_npcController::ExchangeSignals.
// ================================================================

UCLASS()
class MACHINEDESIRING_API U_overlaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Chiamare da BeginPlay di GameMode/PlayerController dopo che il mondo è pronto.
	// Crea il widget, lo aggiunge al viewport, inizializza View 1 con dati statici.
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void SetupOverlay(APlayerController* PC);

	// Entry point per popup handshake — chiamare da A_npcController::ExchangeSignals
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void OnSignalExchanged(int32 IdA, int32 IdB, float ValA, float ValB);

	// Switching viste
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void ShowView(E_overlayView View);

	// Seleziona NPC da ispezionare — attiva View 3 e aggiorna UW_npcInspector
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void InspectNpc(int32 NpcId);

	// Widget principale
	UPROPERTY()
	TObjectPtr<UW_mainOverlay> MainOverlay;

	// Classe Blueprint del widget — assegnare nel GameInstance BP o da codice
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	TSubclassOf<UW_mainOverlay> MainOverlayClass;

private:

	// Aggiorna View 2 (runtime population) ogni 3s
	void TickOverlayData();
	FTimerHandle myTimer_DataRefresh;

	// Carica i dati statici di View 1 da U_populationSubsystem
	void RefreshPopulationPanel();

	// Legge i dati runtime dell'NPC ispezionato e aggiorna UW_npcInspector
	void RefreshNpcInspector();

	// Cache riferimenti (impostati in SetupOverlay)
	UPROPERTY()
	TObjectPtr<U_populationSubsystem> myPopSubsystem;

	UPROPERTY()
	TObjectPtr<A_spawnManager> mySpawnManager;

	int32 myInspectedNpcId = -1;
};
