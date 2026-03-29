#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_populationPanel.generated.h"

class U_populationSubsystem;

// ================================================================
// UW_populationPanel
// View 1 — distribuzioni statiche popolazione (post-generazione).
// Chiamare RefreshData() una volta dopo SetupOverlay.
// Tutti i dati vengono pre-calcolati in bucket istogramma;
// NativePaint disegna solo — nessun calcolo a runtime.
// Blueprint: wrappare in UScrollBox (altezza widget ~2400px).
// ================================================================

UCLASS()
class MACHINEDESIRING_API UW_populationPanel : public UUserWidget
{
	GENERATED_BODY()

public:

	// Chiamato da U_overlaySubsystem dopo lo spawn (e ogni 3s per aggiornamento).
	// SpawnedIds = lista NPC attivamente in gioco (subset del pool 700).
	// Pre-calcola tutti i bucket per NativePaint.
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void RefreshData(U_populationSubsystem* PopSub, const TArray<int32>& SpawnedIDs);

protected:

	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:

	static constexpr int32 BINS = 20;

	bool  b_myDataLoaded = false;
	int32 myPopSize      = 0;

	// §3.1 Dual histograms (My + Desired)
	TArray<int32> H_MyMuscle,  H_DsMuscle;
	TArray<int32> H_MySlim,    H_DsSlim;
	TArray<int32> H_MyBeauty,  H_DsBeauty;
	TArray<int32> H_MyMasc,    H_DsMasc;
	TArray<int32> H_MyHair,    H_DsHair;
	TArray<int32> H_MyAge,     H_DsAge;
	TArray<int32> H_MyBT,      H_DsBT;

	// §3.1 Single histograms (no Desired)
	TArray<int32> H_Queerness, H_Polish,    H_BodyDisplay;
	TArray<int32> H_SexDepth,  H_SexExtrem, H_SelfEsteem;

	// §3.2 Categorical
	TArray<int32>               Cat_Eth;    // 6
	TArray<int32>               Cat_Poz;    // 3
	TArray<TPair<FString,int32>>Cat_Tribe;  // sorted desc by count

	// §3.3 Appeal
	TArray<int32> H_PopVis, H_PopSex, H_PopApp;
	float Gini_Vis = 0.f;

	// §3.4 Pairs scalars
	float myVisCompat    = 0.f;
	float myVisSexCompat = 0.f;
	float myAppCompat    = 0.f;
	float myBtIncompat   = 0.f;
	float myKinkIncompat = 0.f;

	// §3.4 Greedy mutual match simulation
	// Appaiamento greedy su tutte le coppie, sorted by mutual-vis desc.
	// Soglia: VIS/SEX_STANDARD_THRESHOLD (0.35).
	int32 myMatch_Sexual  = 0;  // coppie: vis compat + sex compat
	int32 myMatch_VisOnly = 0;  // coppie: vis compat, sex incompat
	int32 myMatch_None    = 0;  // individui non appaiati

	// §3.5 Attraction counts
	// X = desiderabilità App crescente (bin App-score [0..1], 20 bin)
	// Y = media count attrazioni Out/In assoluta per bin
	TArray<int32> H_AppSorted_OutVis;
	TArray<int32> H_AppSorted_InVis;
	TArray<int32> H_Balance;   // ratio [0, 3], bin=0.15

	// Demand/supply ratio e desire gap — indice: 0=Muscle 1=Slim 2=Beauty
	// 3=Masc 4=Hair 5=Age 6=BT
	float DS[7] = {};
	float DG[7] = {};
};
