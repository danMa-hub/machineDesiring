#include "UW_signalEvent.h"
#include "Rendering/DrawElements.h"
#include "TimerManager.h"

void UW_signalEvent::InitAndShow(int32 IdA, int32 IdB, float ValA, float ValB, float VisAB, float VisBA)
{
	NpcIdA       = IdA;
	NpcIdB       = IdB;
	SignalValueA = ValA;
	SignalValueB = ValB;
	VisScoreAB   = VisAB;
	VisScoreBA   = VisBA;

	// Auto-rimozione dopo 3s
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			myTimer_AutoClose,
			FTimerDelegate::CreateUObject(this, &UW_signalEvent::RemoveFromParent),
			3.f,
			false);
	}
}

int32 UW_signalEvent::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	if (NpcIdA < 0 || NpcIdB < 0) return LayerId;

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	const FVector2D Size = AllottedGeometry.GetLocalSize();

	// Header: A ↔ B
	const FString Header = FString::Printf(TEXT("NPC %d  ↔  NPC %d"), NpcIdA, NpcIdB);
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(4.f, 4.f))),
		Header, Font, ESlateDrawEffect::None, FLinearColor::Yellow);

	// Segnali
	const FString SigLine = FString::Printf(
		TEXT("Signal  A→B: %.2f   B→A: %.2f"), SignalValueA, SignalValueB);
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(4.f, 22.f))),
		SigLine, Font, ESlateDrawEffect::None, FLinearColor::White);

	// VisScore
	const FString VisLine = FString::Printf(
		TEXT("Vis  A→B: %.2f   B→A: %.2f"), VisScoreAB, VisScoreBA);
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId + 2,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(4.f, 38.f))),
		VisLine, Font, ESlateDrawEffect::None, FLinearColor(0.6f, 0.9f, 1.f));

	return LayerId + 2;
}
