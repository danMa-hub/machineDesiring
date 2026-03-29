#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UW_mainOverlay.h"
#include "U_overlaySubsystem.generated.h"

class U_populationSubsystem;
class A_spawnManager;
class U_npcBrainComponent;

// ── Evento accoppiamento — registrato da OnMatingStarted ─────────────
struct F_matchEvent
{
	int32 IdA         = -1;
	int32 IdB         = -1;
	float VisThreshA  = 0.f;   // myVisCurrThreshold di A al momento del match
	float VisThreshB  = 0.f;   // myVisCurrThreshold di B
	float TimeSeconds = 0.f;   // GetWorld()->GetTimeSeconds()
};

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

	virtual void Deinitialize() override;

	// Chiamare da BeginPlay di GameMode/PlayerController dopo che il mondo è pronto.
	// Crea il widget, lo aggiunge al viewport, inizializza View 1 con dati statici.
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void SetupOverlay(APlayerController* PC, TSubclassOf<class UW_mainOverlay> OverlayClass);

	// Entry point per popup handshake — chiamare da A_npcController::ExchangeSignals
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void OnSignalExchanged(int32 IdA, int32 IdB, float ValA, float ValB);

	// Registra un accoppiamento — chiamare da A_npcController::NotifyMatingStarted
	// VisThreshA/B = myVisCurrThreshold dei due NPC al momento del match
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void OnMatingStarted(int32 IdA, int32 IdB, float VisThreshA, float VisThreshB);

	// Switching viste
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void ShowView(E_overlayView View);

	// Seleziona NPC da ispezionare — attiva View 3 e aggiorna UW_npcInspector
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void InspectNpc(int32 NpcId);

	// Forza aggiornamento immediato dei dati popolazione (chiamare prima di ShowView)
	void RefreshPopulationPanel();

	// Widget principale
	UPROPERTY()
	TObjectPtr<UW_mainOverlay> MainOverlay;

	// Classe Blueprint del widget — assegnare nel GameInstance BP o da codice
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	TSubclassOf<UW_mainOverlay> MainOverlayClass;

private:

	// Aggiorna PopulationPanel ogni 3s
	void TickOverlayData();
	FTimerHandle myTimer_DataRefresh;

	// Aggiorna RuntimePanel ogni 1s (match counter + log)
	void RefreshRuntimePanel();
	FTimerHandle myTimer_RuntimeRefresh;

	// Campiona dati NPC ogni 10s (distribuzioni threshold, desired list, GaveUp)
	void SampleRuntimeData();
	FTimerHandle myTimer_RuntimeSample;

	// Legge i dati runtime dell'NPC ispezionato e aggiorna UW_npcInspector
	void RefreshNpcInspector();

	// Cache riferimenti (impostati in SetupOverlay)
	UPROPERTY()
	TObjectPtr<U_populationSubsystem> myPopSubsystem;

	UPROPERTY()
	TObjectPtr<A_spawnManager> mySpawnManager;

	int32 myInspectedNpcId = -1;

	// ── Dati runtime ─────────────────────────────────────────────
	TArray<F_matchEvent> myMatchLog;             // tutti gli accoppiamenti
	TArray<FString>      myMatchLines;           // pre-formattate, più recente prima
	TArray<int32>        myH_VisCurrThresh;      // distribuzione soglie (BINS=20)
	TArray<int32>        myH_DesiredListLen;     // distribuzione lunghezza lista (21 bin)
	float                myAvgGaveUp = 0.f;      // media GaveUp per NPC
};
