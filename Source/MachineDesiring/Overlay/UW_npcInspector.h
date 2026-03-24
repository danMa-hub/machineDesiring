#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_statBar.h"
#include "NpcMemoryTypes.h"
#include "UW_npcInspector.generated.h"

// ================================================================
// UW_npcInspector
// View 3 — ispezione singolo NPC (on-click o selezione da lista).
// Aggiornato su richiesta da U_overlaySubsystem al cambio selezione.
// ================================================================

UCLASS()
class MACHINEDESIRING_API UW_npcInspector : public UUserWidget
{
	GENERATED_BODY()

public:

	// ID NPC correntemente ispezionato (-1 = nessuno)
	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	int32 InspectedNpcId = -1;

	// Aggiorna tutti i dati dell'NPC selezionato
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void RefreshNpc(
		int32 NpcId,
		float PopVisAttraction,
		float PopSexAttraction,
		float PopAppAttraction,
		float VisBalance,
		float VisStartThreshold,
		float VisCurrThreshold,
		float VisMinThreshold,
		float SexStartThreshold,
		float SexCurrThreshold,
		float SexMinThreshold,
		int32 OutVisCount,
		int32 InVisCount,
		int32 DesiredRankCount,
		E_myState CurrentState,
		E_cruisingSubState SubState
	);

	// Deseleziona — pannello vuoto
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void ClearInspection();

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

	float myPopVis          = 0.f;
	float myPopSex          = 0.f;
	float myPopApp          = 0.f;
	float myVisBalance      = 0.f;
	float myVisStart        = 0.f;
	float myVisCurr         = 0.f;
	float myVisMin          = 0.f;
	float mySexStart        = 0.f;
	float mySexCurr         = 0.f;
	float mySexMin          = 0.f;
	int32 myOutVisCount     = 0;
	int32 myInVisCount      = 0;
	int32 myDesiredCount    = 0;
	E_myState        myState    = E_myState::Cruising;
	E_cruisingSubState mySubState = E_cruisingSubState::RandomWalk;
};
