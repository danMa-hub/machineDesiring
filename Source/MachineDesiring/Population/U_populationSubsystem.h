// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "F_npcProfile.h"
#include "U_populationSubsystem.generated.h"


UCLASS()
class MACHINEDESIRING_API U_populationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population")
	int32 PopulationSeed = 42;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Population")
	int32 PopulationSize = 700;

	UPROPERTY(EditAnywhere, Category="Population")
	TObjectPtr<UDataTable> InspectionTable;

	// ── API pubblica ─────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Population")
	void RegeneratePopulation(int32 NewSeed);

	UFUNCTION(BlueprintCallable, Category="Population")
	const F_npcProfile& GetProfile(int32 NpcId) const;

	// Runtime usa questi — CalcVisual/Sexual solo durante pre-generazione
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetVisScore(int32 ObsId, int32 TgtId) const;

	UFUNCTION(BlueprintCallable, Category="Population")
	float GetSexScore(int32 ObsId, int32 TgtId) const;

	// Appeal locale: media attrazioni ricevute dai soli NPC attualmente visibili
	// Chiamare solo quando NearbyNPCs cambia, non ogni tick
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetContextualDesirability(int32 NpcId, const TArray<int32>& VisibleIds) const;

	// Scala mesh normalizzata su min/max popolazione — calcolata post-generazione
	// Range [0.5, 4.0] sull'asse Z
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetMeshScaleZ(int32 NpcId) const;

	// Mantenute per compatibilità Blueprint ma NON usare a runtime NPC
	UFUNCTION(BlueprintCallable, Category="Population")
	float CalcVisualAttraction(int32 ObserverId, int32 TargetId) const;

	UFUNCTION(BlueprintCallable, Category="Population")
	float CalcSexualAttraction(int32 ObserverId, int32 TargetId) const;

	UFUNCTION(BlueprintCallable, Category="Population")
	float CalcAppAttraction(int32 ObserverId, int32 TargetId) const;

	UFUNCTION(BlueprintCallable, Category="Population")
	int32 GetPopulationSize() const { return Profiles.Num(); }

	UFUNCTION(BlueprintCallable, Category="Population")
	void SetInspectionTable(UDataTable* Table);

private:

	TArray<F_npcProfile> Profiles;
	mutable FRandomStream RNG;   // mutable: Calc* sono const ma usano RNG per varianza relazionale

	// ── Matrici pre-calcolate — N×N float, index = i*PopulationSize+j ──
	// Calcolate una volta in GeneratePopulation(), accesso O(1) a runtime
	// N=700: ~3.8MB totali — allocate dopo generazione profili
	TArray<float> VisAttractionMatrix;
	TArray<float> SexAttractionMatrix;

	// Scala mesh per appeal — calcolata post-generazione, normalizzata su min/max
	TArray<float> MeshScaleZCache;

	void GeneratePopulation();
	void GenerateProfile(int32 Id, F_npcProfile& P);
	void GeneratePreferences(F_npcProfile& P);
	void BuildAttractionMatrices();   // chiamata alla fine di GeneratePopulation
	void BuildMeshScaleCache();       // chiamata dopo BuildAttractionMatrices
	void SyncToDataTable();

	// ── RNG helpers ──────────────────────────────────────────────
	float               RandGauss(float Mean, float SD);
	float               RandGaussClamp(float Mean, float SD, float Lo = 0.f, float Hi = 1.f);
	float               RandExpInverse(float Mean, float NoiseFrac);
	float               RandBimodal(float M1, float S1, float W1, float M2, float S2);
	E_npcEthnicity      RandEthnicity();
	E_npcPozStatus      RandPozStatus(float HivRate, float UndetFrac);
	E_npcSafetyPractice RandSafetyPractice(E_npcPozStatus Poz);

	// ── Classificazione ──────────────────────────────────────────
	FString ClassifyTribe(const F_npcProfile& P) const;
	void    AssignKinkTypes(F_npcProfile& P);

	// ── Attrazione helpers ───────────────────────────────────────
	float EthDesirability(E_npcEthnicity Eth) const;
	float PozDesirability(E_npcPozStatus Poz) const;
	float Clamp01(float V) const { return FMath::Clamp(V, 0.f, 1.f); }
};
