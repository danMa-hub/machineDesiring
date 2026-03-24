#include "UW_populationPanel.h"
#include "Rendering/DrawElements.h"

void UW_populationPanel::RefreshData(
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
	float InDesireGap_BottomTop)
{
	myVisCompatRate       = InVisCompatRate;
	myVisSexCompatRate    = InVisSexCompatRate;
	myAppCompatRate       = InAppCompatRate;
	myGiniPolarization    = InGiniPolarization;
	myBtIncompatShare     = InBtIncompatShare;
	myKinkIncompatShare   = InKinkIncompatShare;
	myDemandSupply_Muscle = InDemandSupply_Muscle;
	myDemandSupply_Beauty = InDemandSupply_Beauty;
	myDemandSupply_BT     = InDemandSupply_BottomTop;
	myDesireGap_Muscle    = InDesireGap_Muscle;
	myDesireGap_Beauty    = InDesireGap_Beauty;
	myDesireGap_Age       = InDesireGap_Age;
	myDesireGap_BT        = InDesireGap_BottomTop;
	b_myDataLoaded        = true;
}

int32 UW_populationPanel::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	if (!b_myDataLoaded) return LayerId;

	const FSlateFontInfo FontSm = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	const FSlateFontInfo FontLg = FCoreStyle::GetDefaultFontStyle("Bold", 11);
	const FVector2D Size = AllottedGeometry.GetLocalSize();

	// Titolo
	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, 6.f))),
		TEXT("POPULATION — Static"), FontLg, ESlateDrawEffect::None, FLinearColor::White);

	// Pairs compatibility
	float Y = 28.f;
	auto DrawRow = [&](const FString& LabelStr, float Val, FLinearColor Color)
	{
		const FString Line = FString::Printf(TEXT("%-32s %.1f%%"), *LabelStr, Val * 100.f);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, Y))),
			Line, FontSm, ESlateDrawEffect::None, Color);
		Y += 14.f;
	};

	DrawRow(TEXT("Vis compatible pairs"),         myVisCompatRate,     FLinearColor::Green);
	DrawRow(TEXT("Vis+Sex compatible (cruise)"),  myVisSexCompatRate,  FLinearColor(0.4f,1.f,0.4f));
	DrawRow(TEXT("App compatible pairs"),         myAppCompatRate,     FLinearColor(0.6f,0.9f,1.f));
	DrawRow(TEXT("BT incompatibility (vis-match)"),myBtIncompatShare,  FLinearColor(1.f,0.5f,0.2f));
	DrawRow(TEXT("Kink incompatibility"),         myKinkIncompatShare, FLinearColor(1.f,0.4f,0.4f));

	Y += 6.f;
	DrawRow(TEXT("Vis Gini polarization"),        myGiniPolarization,  FLinearColor(0.9f,0.9f,0.5f));

	Y += 6.f;
	// Demand/supply
	auto DemandColor = [](float V) -> FLinearColor {
		if (V > 1.1f) return FLinearColor(1.f,0.3f,0.3f);
		if (V < 0.9f) return FLinearColor(0.3f,0.5f,1.f);
		return FLinearColor(0.3f,1.f,0.3f);
	};
	const FString DSMuscle = FString::Printf(TEXT("D/S Muscle  %.2f"), myDemandSupply_Muscle);
	const FString DSBeauty = FString::Printf(TEXT("D/S Beauty  %.2f"), myDemandSupply_Beauty);
	const FString DSBT     = FString::Printf(TEXT("D/S BottomTop  %.2f"), myDemandSupply_BT);

	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, Y))),
		DSMuscle, FontSm, ESlateDrawEffect::None, DemandColor(myDemandSupply_Muscle)); Y += 14.f;
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, Y))),
		DSBeauty, FontSm, ESlateDrawEffect::None, DemandColor(myDemandSupply_Beauty)); Y += 14.f;
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D(8.f, Y))),
		DSBT, FontSm, ESlateDrawEffect::None, DemandColor(myDemandSupply_BT)); Y += 14.f;

	return LayerId + 1;
}
