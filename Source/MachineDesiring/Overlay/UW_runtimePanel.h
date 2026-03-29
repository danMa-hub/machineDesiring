#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_runtimePanel.generated.h"

// ================================================================
// UW_runtimePanel
// View 2 — dati runtime della simulazione.
// Aggiornata ogni secondo da U_overlaySubsystem.
// Mostra: timer simulazione, log accoppiamenti (con soglie e timestamp),
// distribuzione vis_curr_threshold, distribuzione lista desiderati,
// media GaveUp per NPC.
// Blueprint: wrappare in UScrollBox (altezza widget ~1800px).
// ================================================================

UCLASS()
class MACHINEDESIRING_API UW_runtimePanel : public UUserWidget
{
	GENERATED_BODY()

public:

	// Chiamato ogni secondo da U_overlaySubsystem.
	// MatchLines: lista pre-formattata, evento più recente per primo.
	// VisCurrThreshHist / DesiredListHist: aggiornati ogni 10s.
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void RefreshRuntimeData(
		float                  SimTime,
		int32                  TotalMatches,
		const TArray<FString>& MatchLines,
		const TArray<int32>&   VisCurrThreshHist,
		const TArray<int32>&   DesiredListHist,
		float                  AvgGaveUp);

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

	static constexpr int32 BINS          = 20;
	static constexpr int32 MAX_LOG_LINES = 30;

	bool           b_myDataLoaded = false;
	float          mySimTime      = 0.f;
	int32          myTotalMatches = 0;
	float          myAvgGaveUp    = 0.f;

	TArray<FString> myMatchLines;
	TArray<int32>   H_VisCurrThresh;   // BINS=20, range [0,1]
	TArray<int32>   H_DesiredListLen;  // 21 bins (indice = lunghezza lista 0..20)
};
