#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_statBar.h"
#include "UW_populationPanel.generated.h"

// ================================================================
// UW_populationPanel
// View 1 — distribuzioni statiche popolazione (post-generazione).
// Dati fissi: aggiornati una sola volta da U_overlaySubsystem.
// Rendering via NativePaint. Blueprint subclass per posizionamento.
// ================================================================

UCLASS()
class MACHINEDESIRING_API UW_populationPanel : public UUserWidget
{
	GENERATED_BODY()

public:

	// Aggiorna tutti i dati — chiamato una volta da U_overlaySubsystem dopo la generazione
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void RefreshData(
		float InVisCompatRate,
		float InVisSexCompatRate,
		float InAppCompatRate,
		float InGiniPolarization,
		float InBtIncompatShare,
		float InKinkIncompatShare,
		float InDemandSupply_Muscle,
		float InDemandSupply_Beauty,
		float InDemandSupply_BottomTop,
		float InDesireGap_Muscle,
		float InDesireGap_Beauty,
		float InDesireGap_Age,
		float InDesireGap_BottomTop
	);

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

	// Cache dati per NativePaint (const — non modificabili nel paint)
	float myVisCompatRate        = 0.f;
	float myVisSexCompatRate     = 0.f;
	float myAppCompatRate        = 0.f;
	float myGiniPolarization     = 0.f;
	float myBtIncompatShare      = 0.f;
	float myKinkIncompatShare    = 0.f;
	float myDemandSupply_Muscle  = 0.f;
	float myDemandSupply_Beauty  = 0.f;
	float myDemandSupply_BT      = 0.f;
	float myDesireGap_Muscle     = 0.f;
	float myDesireGap_Beauty     = 0.f;
	float myDesireGap_Age        = 0.f;
	float myDesireGap_BT         = 0.f;

	bool b_myDataLoaded = false;
};
