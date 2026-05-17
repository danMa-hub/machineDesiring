#include "UW_statBar.h"
#include "Rendering/DrawElements.h"

int32 UW_statBar::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float BarHeight = FMath::Max(Size.Y - 16.f, 4.f);  // spazio per label
	const float BarY      = 14.f;

	// Sfondo
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(Size.X, BarHeight), FSlateLayoutTransform(FVector2D(0.f, BarY))),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		BackgroundColor);

	// Barra riempita
	const float FillW = FMath::Clamp(Value, 0.f, 1.f) * Size.X;
	if (FillW > 0.f)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(FillW, BarHeight), FSlateLayoutTransform(FVector2D(0.f, BarY))),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			BarColor);
	}

	// Label
	if (!Label.IsEmpty())
	{
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 9);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(FVector2D(Size.X, 14.f), FSlateLayoutTransform(FVector2D(0.f, 0.f))),
			Label,
			Font,
			ESlateDrawEffect::None,
			FLinearColor::White);
	}

	return LayerId + 2;
}
