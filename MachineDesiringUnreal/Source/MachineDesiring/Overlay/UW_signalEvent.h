#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_signalEvent.generated.h"

// ================================================================
// UW_signalEvent
// Popup temporaneo triggered da A_npcController::ExchangeSignals.
// Mostra i dati dell'handshake A↔B. Auto-rimozione dopo 3s.
// ================================================================

UCLASS()
class MACHINEDESIRING_API UW_signalEvent : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	int32 NpcIdA = -1;

	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	int32 NpcIdB = -1;

	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	float SignalValueA = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	float SignalValueB = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	float VisScoreAB = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	float VisScoreBA = 0.f;

	// Inizializza i dati e avvia il timer di auto-chiusura (3s)
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void InitAndShow(int32 IdA, int32 IdB, float ValA, float ValB, float VisAB, float VisBA);

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

	FTimerHandle myTimer_AutoClose;
};
