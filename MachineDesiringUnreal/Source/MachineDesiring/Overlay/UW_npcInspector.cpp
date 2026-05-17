#include "UW_npcInspector.h"
#include "Rendering/DrawElements.h"

void UW_npcInspector::RefreshNpc(
	int32 NpcId,
	float PopVisAttraction, float PopSexAttraction, float PopAppAttraction,
	float VisBalance,
	float VisStartThreshold, float VisCurrThreshold, float VisMinThreshold,
	float SexStartThreshold, float SexCurrThreshold, float SexMinThreshold,
	int32 OutVisCount, int32 InVisCount,
	int32 DesiredRankCount,
	E_myState CurrentState, E_cruisingSubState SubState)
{
	InspectedNpcId = NpcId;
	myPopVis       = PopVisAttraction;
	myPopSex       = PopSexAttraction;
	myPopApp       = PopAppAttraction;
	myVisBalance   = VisBalance;
	myVisStart     = VisStartThreshold;
	myVisCurr      = VisCurrThreshold;
	myVisMin       = VisMinThreshold;
	mySexStart     = SexStartThreshold;
	mySexCurr      = SexCurrThreshold;
	mySexMin       = SexMinThreshold;
	myOutVisCount  = OutVisCount;
	myInVisCount   = InVisCount;
	myDesiredCount = DesiredRankCount;
	myState        = CurrentState;
	mySubState     = SubState;
}

void UW_npcInspector::ClearInspection()
{
	InspectedNpcId = -1;
}

int32 UW_npcInspector::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FSlateFontInfo FontSm = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	const FSlateFontInfo FontLg = FCoreStyle::GetDefaultFontStyle("Bold", 11);
	const FVector2D Size = AllottedGeometry.GetLocalSize();

	if (InspectedNpcId < 0)
	{
		FSlateDrawElement::MakeText(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, 8.f))),
			TEXT("No NPC selected"), FontSm, ESlateDrawEffect::None, FLinearColor(0.5f,0.5f,0.5f));
		return LayerId;
	}

	float Y = 6.f;
	auto DrawLine = [&](const FString& Text, FLinearColor Color = FLinearColor::White)
	{
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, Y))),
			Text, FontSm, ESlateDrawEffect::None, Color);
		Y += 14.f;
	};

	// Titolo
	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, Y))),
		FString::Printf(TEXT("NPC  %d"), InspectedNpcId),
		FontLg, ESlateDrawEffect::None, FLinearColor::Yellow);
	Y += 18.f;

	// Stato FSM
	const FString StateStr = (myState == E_myState::Cruising) ? TEXT("Cruising") :
	                         (myState == E_myState::Mating)   ? TEXT("Mating")   : TEXT("Latency");
	DrawLine(FString::Printf(TEXT("State: %s"), *StateStr), FLinearColor(0.6f,1.f,0.6f));

	Y += 4.f;

	// Appeal
	DrawLine(FString::Printf(TEXT("Pop Vis Appeal:  %.3f"), myPopVis));
	DrawLine(FString::Printf(TEXT("Pop Sex Appeal:  %.3f"), myPopSex));
	DrawLine(FString::Printf(TEXT("Pop App Appeal:  %.3f"), myPopApp));
	DrawLine(FString::Printf(TEXT("Vis Balance:     %.2f  (in %d / out %d)"),
		myVisBalance, myInVisCount, myOutVisCount));

	Y += 4.f;

	// Soglie vis — three-point bar testuale
	DrawLine(FString::Printf(TEXT("Vis  min %.2f | curr %.2f | start %.2f"),
		myVisMin, myVisCurr, myVisStart),
		myVisCurr < 0.35f ? FLinearColor(1.f,0.5f,0.2f) : FLinearColor::White);

	DrawLine(FString::Printf(TEXT("Sex  min %.2f | curr %.2f | start %.2f"),
		mySexMin, mySexCurr, mySexStart));

	Y += 4.f;

	// Desired rank
	DrawLine(FString::Printf(TEXT("Desired rank:  %d entries"), myDesiredCount));

	return LayerId + 1;
}
