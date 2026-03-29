#include "UW_runtimePanel.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

// ──────────────────────────────────────────────────────────────────────
// Helpers di disegno (statici a questo translation unit)
// ──────────────────────────────────────────────────────────────────────

static void RT_Box(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float Y, float W, float H,
	const FLinearColor& Col)
{
	if (W <= 0.f || H <= 0.f) return;
	static const FSlateColorBrush s_Brush(FLinearColor::White);
	FSlateDrawElement::MakeBox(Out, Layer,
		G.ToPaintGeometry(FVector2D(W, H), FSlateLayoutTransform(FVector2D(X, Y))),
		&s_Brush, ESlateDrawEffect::None, Col);
}

static void RT_Text(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float Y,
	const FString& Str, const FSlateFontInfo& Font,
	const FLinearColor& Col)
{
	FSlateDrawElement::MakeText(Out, Layer,
		G.ToPaintGeometry(G.GetLocalSize(), FSlateLayoutTransform(FVector2D(X, Y))),
		Str, Font, ESlateDrawEffect::None, Col);
}

static void RT_SectionHeader(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float& Y,
	const FString& Title, const FSlateFontInfo& Font)
{
	RT_Box(Out, Layer, G, X, Y + 8.f, G.GetLocalSize().X - X * 2.f, 1.f,
		FLinearColor(0.40f, 0.40f, 0.40f, 1.f));
	Y += 12.f;
	RT_Text(Out, Layer, G, X, Y, Title, Font, FLinearColor(0.90f, 0.90f, 0.55f, 1.f));
	Y += 20.f;
}

static void RT_Histogram(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float Y, float W, float H,
	const TArray<int32>& A, FLinearColor Col,
	const FSlateFontInfo* ScaleFont = nullptr)
{
	if (A.Num() == 0) return;
	const int32 Bins = A.Num();
	int32 MaxVal = 1;
	for (int32 V : A) MaxVal = FMath::Max(MaxVal, V);
	const float BinW = W / Bins;

	RT_Box(Out, Layer, G, X, Y, W, H, FLinearColor(0.12f, 0.12f, 0.12f, 1.f));
	Col.A = 0.85f;
	for (int32 i = 0; i < Bins; ++i)
	{
		const float BH = H * (float)A[i] / MaxVal;
		if (BH > 0.5f)
			RT_Box(Out, Layer + 1, G, X + i * BinW + 1.f, Y + H - BH, BinW - 2.f, BH, Col);
	}
	if (ScaleFont)
	{
		RT_Text(Out, Layer + 2, G, X + W - 24.f, Y + 1.f,
			FString::FromInt(MaxVal), *ScaleFont,
			FLinearColor(0.52f, 0.52f, 0.52f, 0.85f));
	}
}

// ──────────────────────────────────────────────────────────────────────
// RefreshRuntimeData
// ──────────────────────────────────────────────────────────────────────

void UW_runtimePanel::RefreshRuntimeData(
	float                  SimTime,
	int32                  TotalMatches,
	const TArray<FString>& MatchLines,
	const TArray<int32>&   VisCurrThreshHist,
	const TArray<int32>&   DesiredListHist,
	float                  AvgGaveUp)
{
	mySimTime      = SimTime;
	myTotalMatches = TotalMatches;
	myAvgGaveUp    = AvgGaveUp;

	myMatchLines        = MatchLines;
	H_VisCurrThresh     = VisCurrThreshHist;
	H_DesiredListLen    = DesiredListHist;

	b_myDataLoaded = true;
	Invalidate(EInvalidateWidgetReason::Paint);
}

// ──────────────────────────────────────────────────────────────────────
// NativePaint — solo disegno, nessun calcolo
// ──────────────────────────────────────────────────────────────────────

int32 UW_runtimePanel::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FSlateFontInfo FontLg  = FCoreStyle::GetDefaultFontStyle("Regular", 16);
	const FSlateFontInfo FontMed = FCoreStyle::GetDefaultFontStyle("Bold",    12);
	const FSlateFontInfo FontSm  = FCoreStyle::GetDefaultFontStyle("Regular",  9);
	const FLinearColor   ColDim(0.60f, 0.60f, 0.60f, 1.f);

	const float WW  = AllottedGeometry.GetLocalSize().X;
	const float MG  = 40.f;
	const float GAP = 20.f;
	const float BAR_H = 80.f;
	const int32 L   = LayerId;
	float Y = MG;

	// ── Header: timer simulazione (sempre visibile, non dipende dai dati) ─
	float SimTimeLive = mySimTime;
	if (const UWorld* W = GetWorld()) SimTimeLive = W->GetTimeSeconds();
	const int32 Mins = FMath::FloorToInt(SimTimeLive / 60.f);
	const int32 Secs = FMath::FloorToInt(SimTimeLive) % 60;
	RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		FString::Printf(TEXT("RUNTIME DATA   |   SIM: %02d:%02d s"), Mins, Secs),
		FontLg, FLinearColor::White);
	Y += 28.f;

	// ── Dati non ancora caricati ───────────────────────────────────
	if (!b_myDataLoaded)
	{
		RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			TEXT("(in attesa del primo aggiornamento dati...)"),
			FontMed, ColDim);
		return L + 1;
	}

	// ── §4.1 ACCOPPIAMENTI ────────────────────────────────────────
	RT_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("4.1  ACCOPPIAMENTI"), FontMed);

	RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		FString::Printf(TEXT("Totale accoppiamenti:  %d"), myTotalMatches),
		FontMed, FLinearColor(0.25f, 0.92f, 0.42f, 1.f));
	Y += 20.f;

	// Log eventi: intestazione colonne
	RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("   Tempo      NPC A ↔ B       vis_thresh A    vis_thresh B"),
		FontSm, ColDim);
	Y += 14.f;

	// Lista eventi (più recente in cima), max MAX_LOG_LINES
	const int32 ShowN = FMath::Min(myMatchLines.Num(), MAX_LOG_LINES);
	for (int32 i = 0; i < ShowN; ++i)
	{
		const float Alpha = 1.f - 0.6f * (float)i / FMath::Max(ShowN - 1, 1);
		RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			myMatchLines[i], FontSm,
			FLinearColor(0.80f, 0.88f, 0.80f, Alpha));
		Y += 14.f;
	}

	if (myMatchLines.Num() == 0)
	{
		RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			TEXT("  (nessun accoppiamento ancora)"), FontSm, ColDim);
		Y += 14.f;
	}
	Y += GAP;

	// ── §4.2 DISTRIBUZIONE VIS_CURR_THRESHOLD ────────────────────
	RT_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("4.2  DISTRIBUZIONE  vis_curr_threshold  (campionata ogni 10s)"), FontMed);

	RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("← basso [0.05]                                  alto [0.35] →"),
		FontSm, ColDim);
	Y += 13.f;

	if (H_VisCurrThresh.Num() > 0)
	{
		RT_Histogram(OutDrawElements, L, AllottedGeometry,
			MG, Y, WW - MG * 2.f, BAR_H,
			H_VisCurrThresh, FLinearColor(0.35f, 0.75f, 0.95f, 1.f), &FontSm);
		Y += BAR_H + GAP;
	}
	else
	{
		RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			TEXT("  (in attesa del primo campionamento...)"), FontSm, ColDim);
		Y += 20.f + GAP;
	}

	// ── §4.3 DISTRIBUZIONE LUNGHEZZA LISTA DESIDERATI ────────────
	RT_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("4.3  LISTA DESIDERATI — lunghezza per NPC  (campionata ogni 10s)"), FontMed);

	RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("← lista vuota [0]                              piena [20] →"),
		FontSm, ColDim);
	Y += 13.f;

	if (H_DesiredListLen.Num() > 0)
	{
		RT_Histogram(OutDrawElements, L, AllottedGeometry,
			MG, Y, WW - MG * 2.f, BAR_H,
			H_DesiredListLen, FLinearColor(0.95f, 0.65f, 0.25f, 1.f), &FontSm);
		Y += BAR_H + GAP;
	}
	else
	{
		RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			TEXT("  (in attesa del primo campionamento...)"), FontSm, ColDim);
		Y += 20.f + GAP;
	}

	// ── §4.4 MEDIA GAVEUP ─────────────────────────────────────────
	RT_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("4.4  RIFIUTATI / ABBANDONATI  (campionato ogni 10s)"), FontMed);

	RT_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		FString::Printf(TEXT("Media GaveUp per NPC:  %.1f"), myAvgGaveUp),
		FontMed, FLinearColor(1.f, 0.42f, 0.18f, 1.f));
	Y += 20.f + MG;

	return L + 3;
}

