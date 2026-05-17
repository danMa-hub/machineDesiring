#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_statBar.generated.h"

// ================================================================
// UW_statBar
// Widget atomico riutilizzabile — disegna una singola barra [0,1].
// Usato come componente da UW_populationPanel e UW_npcInspector.
// Rendering via NativePaint (nessun asset aggiuntivo).
// ================================================================

UCLASS()
class MACHINEDESIRING_API UW_statBar : public UUserWidget
{
	GENERATED_BODY()

public:

	// Valore corrente [0, 1]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	float Value = 0.f;

	// Label testuale sopra la barra (vuoto = nessun testo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	FString Label;

	// Colore della barra
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	FLinearColor BarColor = FLinearColor::White;

	// Colore sfondo barra
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	FLinearColor BackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.8f);

protected:

	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
};
