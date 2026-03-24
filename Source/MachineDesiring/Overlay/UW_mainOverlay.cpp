#include "UW_mainOverlay.h"

void UW_mainOverlay::NativeConstruct()
{
	Super::NativeConstruct();
	HideAll();
}

void UW_mainOverlay::ShowView(E_overlayView View)
{
	ActiveView = View;

	if (PopulationPanel)
		PopulationPanel->SetVisibility(
			View == E_overlayView::PopulationPanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (NpcInspector)
		NpcInspector->SetVisibility(
			View == E_overlayView::NpcInspector ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UW_mainOverlay::HideAll()
{
	ActiveView = E_overlayView::None;
	if (PopulationPanel) PopulationPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (NpcInspector)    NpcInspector->SetVisibility(ESlateVisibility::Collapsed);
}

void UW_mainOverlay::ShowSignalEvent(int32 IdA, int32 IdB, float ValA, float ValB, float VisAB, float VisBA)
{
	if (!SignalEventClass) return;

	UW_signalEvent* Popup = CreateWidget<UW_signalEvent>(GetOwningPlayer(), SignalEventClass);
	if (Popup)
	{
		Popup->InitAndShow(IdA, IdB, ValA, ValB, VisAB, VisBA);
		Popup->AddToViewport(10);  // Z-order alto: sopra tutto
	}
}
