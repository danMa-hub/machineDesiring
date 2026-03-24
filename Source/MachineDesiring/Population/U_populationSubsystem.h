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

	// ── Costanti globali di riferimento ───────────────────────────
	// Parametro artistico calibrato. TODO: verifica vs letteratura mate choice
	// (Conroy-Beam, Buston & Emlen 2003). Usato per tutte le metriche
	// population-level count. Le soglie per-NPC (myVisCurrThreshold) partono
	// sopra questo valore e possono scendere sotto per pressione di frustrazione.
	static constexpr float VIS_STANDARD_THRESHOLD = 0.35f;
	static constexpr float SEX_STANDARD_THRESHOLD = 0.35f;

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

	// ── §1.2/1.3 — Getters cached vectors ────────────────────────

	// AppAttractionMatrix: CalcAppAttraction() single-pass, tutti gli 11 variabili
	// con pesi normalizzati da letteratura. Metrica globale raccomandata,
	// sostituisce la formula arbitraria 0.60×vis + 0.40×sex.
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetAppScore(int32 ObsId, int32 TgtId) const;

	// myPop_visAttraction: media attrazioni visive ricevute dall'intera popolazione
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetPopVisAttraction(int32 NpcId) const;

	// myPop_sexAttraction: media attrazioni sessuali ricevute
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetPopSexAttraction(int32 NpcId) const;

	// myPop_appAttraction: media attrazioni app-mode ricevute
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetPopAppAttraction(int32 NpcId) const;

	// myOut_visAttractionCount: quanti NPC questo NPC trova attraenti visivamente
	// Calcolato su VIS_STANDARD_THRESHOLD (0.35 fisso, NON myVisCurrThreshold)
	UFUNCTION(BlueprintCallable, Category="Population")
	int32 GetOutVisAttractionCount(int32 NpcId) const;

	// myIn_visAttractionCount: quanti NPC trovano questo NPC attraente visivamente
	UFUNCTION(BlueprintCallable, Category="Population")
	int32 GetInVisAttractionCount(int32 NpcId) const;

	// my_visAttractionBalance: myIn / myOut — posizione di mercato per-NPC
	// >1 = desiderato da più di quanti ne desidera, <1 = frustrazione strutturale
	UFUNCTION(BlueprintCallable, Category="Population")
	float GetVisAttractionBalance(int32 NpcId) const;

	// ── §1.4 — Getters metriche scalari popolazione ───────────────

	// Gini su PopVisAttractionCache — 0=uguale, 1=tutto concentrato su un NPC
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetPopVisAttractionPolarization() const { return PopVisAttractionPolarization; }

	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetPairsVisCompatibilityRate() const { return PairsVisCompatibilityRate; }

	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetPairsVisSexCompatibilityRate_cruise() const { return PairsVisSexCompatibilityRate_cruise; }

	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetPairsFullCompatibilityRateApp() const { return PairsFullCompatibilityRateApp; }

	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetPairsVisMatchedBottomTopIncompatibilityShare_cruise() const { return PairsVisMatchedBottomTopIncompatibilityShare_cruise; }

	// Kink incompatibility threshold: slider-configurabile, default 0.80
	// Una coppia è kink-incompatibile se ENTRAMBE le direzioni superano la soglia
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetPairsKinkIncompatibilityShare() const { return PairsKinkIncompatibilityShare; }

	// Demand/supply ratio per variabile: sum(myDesired_X) / sum(MyX)
	// >1 = undersupply, <1 = oversupply
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDemandSupplyRatio_Muscle()      const { return PopDemandSupplyRatio_Muscle; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDemandSupplyRatio_Slim()        const { return PopDemandSupplyRatio_Slim; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDemandSupplyRatio_Beauty()      const { return PopDemandSupplyRatio_Beauty; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDemandSupplyRatio_Masculinity() const { return PopDemandSupplyRatio_Masculinity; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDemandSupplyRatio_Hair()        const { return PopDemandSupplyRatio_Hair; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDemandSupplyRatio_Age()         const { return PopDemandSupplyRatio_Age; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDemandSupplyRatio_BottomTop()   const { return PopDemandSupplyRatio_BottomTop; }

	// Desire gap per variabile: mean(myDesired_X - MyX)
	// Positivo = popolazione desidera più di quanto possiede
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDesireGap_Muscle()      const { return PopDesireGap_Muscle; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDesireGap_Slim()        const { return PopDesireGap_Slim; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDesireGap_Beauty()      const { return PopDesireGap_Beauty; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDesireGap_Masculinity() const { return PopDesireGap_Masculinity; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDesireGap_Hair()        const { return PopDesireGap_Hair; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDesireGap_Age()         const { return PopDesireGap_Age; }
	UFUNCTION(BlueprintCallable, Category="Population|Metrics")
	float GetDesireGap_BottomTop()   const { return PopDesireGap_BottomTop; }

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

	// §1.2 — AppAttractionMatrix: CalcAppAttraction() single-pass, tutti gli 11 variabili
	// con pesi normalizzati da letteratura (BottomTop dominante a 0.203).
	// Metrica globale raccomandata, sostituisce 0.60×vis + 0.40×sex.
	TArray<float> AppAttractionMatrix;

	// Scala mesh per appeal — calcolata post-generazione, normalizzata su min/max
	TArray<float> MeshScaleZCache;

	// §1.3 — Cached vectors per-NPC (O(N), calcolati una volta post-generazione)

	// myPop_visAttraction: media attrazioni visive ricevute da tutta la popolazione
	// Nota: già calcolato in BuildMeshScaleCache come PopVis[], ora esposto
	TArray<float> PopVisAttractionCache;

	// myPop_sexAttraction: media attrazioni sessuali ricevute
	TArray<float> PopSexAttractionCache;

	// myPop_appAttraction: media attrazioni app-mode ricevute
	TArray<float> PopAppAttractionCache;

	// myOut_visAttractionCount: quanti NPC questo NPC trova attraenti (VIS_STANDARD_THRESHOLD)
	// NON usa myVisCurrThreshold adattiva — confronto population-level richiede soglia fissa
	TArray<int32> OutVisAttractionCount;

	// myIn_visAttractionCount: quanti NPC trovano questo NPC attraente (VIS_STANDARD_THRESHOLD)
	TArray<int32> InVisAttractionCount;

	// my_visAttractionBalance: myIn / myOut — posizione strutturale di mercato
	TArray<float> VisAttractionBalance;

	// §1.4 — Metriche scalari popolazione (float singoli, calcolati una volta)

	float PopVisAttractionPolarization      = 0.f;  // Gini su PopVisAttractionCache
	float PairsVisCompatibilityRate         = 0.f;  // % coppie con match visivo mutuo
	float PairsVisSexCompatibilityRate_cruise      = 0.f;  // % coppie con match vis+sex mutuo
	float PairsFullCompatibilityRateApp     = 0.f;  // % coppie con match app-mode mutuo
	float PairsVisMatchedBottomTopIncompatibilityShare_cruise= 0.f;  // quota vis-matched che fallisce per BT
	float PairsKinkIncompatibilityShare     = 0.f;  // quota che fallisce per kink mismatch

	float PopDemandSupplyRatio_Muscle       = 0.f;
	float PopDemandSupplyRatio_Slim         = 0.f;
	float PopDemandSupplyRatio_Beauty       = 0.f;
	float PopDemandSupplyRatio_Masculinity  = 0.f;
	float PopDemandSupplyRatio_Hair         = 0.f;
	float PopDemandSupplyRatio_Age          = 0.f;
	float PopDemandSupplyRatio_BottomTop    = 0.f;

	float PopDesireGap_Muscle               = 0.f;
	float PopDesireGap_Slim                 = 0.f;
	float PopDesireGap_Beauty               = 0.f;
	float PopDesireGap_Masculinity          = 0.f;
	float PopDesireGap_Hair                 = 0.f;
	float PopDesireGap_Age                  = 0.f;
	float PopDesireGap_BottomTop            = 0.f;

	void GeneratePopulation();
	void GenerateProfile(int32 Id, F_npcProfile& P);
	void GeneratePreferences(F_npcProfile& P);
	void BuildAttractionMatrices();   // chiamata alla fine di GeneratePopulation
	void BuildMeshScaleCache();       // chiamata dopo BuildAttractionMatrices
	void BuildPopulationMetrics();    // chiamata dopo BuildMeshScaleCache
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
