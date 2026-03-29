#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UW_populationPanel.h"
#include "UW_runtimePanel.h"
#include "UW_npcInspector.h"
#include "UW_signalEvent.h"
#include "UW_mainOverlay.generated.h"

// Vista attiva nel pannello overlay
UENUM(BlueprintType)
enum class E_overlayView : uint8
{
	None            UMETA(DisplayName="None"),
	PopulationPanel UMETA(DisplayName="PopulationPanel"),
	RuntimePanel    UMETA(DisplayName="RuntimePanel"),
	NpcInspector    UMETA(DisplayName="NpcInspector"),
};

// ================================================================
// UW_mainOverlay
// Contenitore principale overlay. Gestisce switching tra View 1 e 3.
// View 4 (SignalEvent) è istanziata dinamicamente e sovrapposta.
// Blueprint subclass assegna i sottowidget e definisce il layout.
// ================================================================

UCLASS()
class MACHINEDESIRING_API UW_mainOverlay : public UUserWidget
{
	GENERATED_BODY()

public:

	// Mostra la vista richiesta, nasconde le altre
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void ShowView(E_overlayView View);

	// Nasconde tutte le viste
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void HideAll();

	// Istanzia e mostra un popup handshake
	UFUNCTION(BlueprintCallable, Category="Overlay")
	void ShowSignalEvent(int32 IdA, int32 IdB, float ValA, float ValB, float VisAB, float VisBA);

	// Vista corrente
	UPROPERTY(BlueprintReadOnly, Category="Overlay")
	E_overlayView ActiveView = E_overlayView::None;

	// Sottowidget — opzionalmente legati al Blueprint Designer (BindWidgetOptional).
	// Se presenti nel Designer con nome esatto → binding automatico.
	// Se assenti → U_overlaySubsystem li crea programmaticamente come fallback.
	UPROPERTY(BlueprintReadWrite, meta=(BindWidgetOptional), Category="Overlay")
	TObjectPtr<UW_populationPanel> PopulationPanel;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidgetOptional), Category="Overlay")
	TObjectPtr<UW_runtimePanel> RuntimePanel;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidgetOptional), Category="Overlay")
	TObjectPtr<UW_npcInspector> NpcInspector;

	// Classe per i popup segnale — assegnare in Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay")
	TSubclassOf<UW_signalEvent> SignalEventClass;

protected:

	virtual void NativeConstruct() override;
};
