#include "UW_populationPanel.h"
#include "U_populationSubsystem.h"
#include "F_npcProfile.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

// ──────────────────────────────────────────────────────────────────────
// Helpers di disegno
// ──────────────────────────────────────────────────────────────────────

static void PD_Box(
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

static void PD_Text(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float Y,
	const FString& Str, const FSlateFontInfo& Font,
	const FLinearColor& Col)
{
	FSlateDrawElement::MakeText(Out, Layer,
		G.ToPaintGeometry(G.GetLocalSize(), FSlateLayoutTransform(FVector2D(X, Y))),
		Str, Font, ESlateDrawEffect::None, Col);
}

static void PD_SectionHeader(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float& Y,
	const FString& Title, const FSlateFontInfo& Font)
{
	PD_Box(Out, Layer, G, X, Y + 8.f, G.GetLocalSize().X - X * 2.f, 1.f,
		FLinearColor(0.40f, 0.40f, 0.40f, 1.f));
	Y += 12.f;
	PD_Text(Out, Layer, G, X, Y, Title, Font, FLinearColor(0.90f, 0.90f, 0.55f, 1.f));
	Y += 18.f;
}

// Istogramma verticale con etichetta scala interna opzionale (max + max/2)
static void PD_Histogram(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float Y, float W, float H,
	const TArray<int32>& A, FLinearColor ColA,
	const TArray<int32>* B = nullptr, FLinearColor ColB = FLinearColor::White,
	const FSlateFontInfo* ScaleFont = nullptr)
{
	if (A.Num() == 0) return;
	const int32 Bins = A.Num();

	int32 MaxVal = 1;
	for (int32 V : A) MaxVal = FMath::Max(MaxVal, V);
	if (B) for (int32 V : *B) MaxVal = FMath::Max(MaxVal, V);

	const float BinW = W / Bins;

	PD_Box(Out, Layer, G, X, Y, W, H, FLinearColor(0.12f, 0.12f, 0.12f, 1.f));

	ColA.A = 0.85f;
	for (int32 i = 0; i < Bins; ++i)
	{
		const float BH = H * (float)A[i] / MaxVal;
		if (BH > 0.5f)
			PD_Box(Out, Layer + 1, G, X + i * BinW + 1.f, Y + H - BH, BinW - 2.f, BH, ColA);
	}

	if (B && B->Num() == Bins)
	{
		ColB.A = 0.55f;
		for (int32 i = 0; i < Bins; ++i)
		{
			const float BH = H * (float)(*B)[i] / MaxVal;
			if (BH > 0.5f)
				PD_Box(Out, Layer + 2, G, X + i * BinW + 1.f, Y + H - BH, BinW - 2.f, BH, ColB);
		}
	}

	// Scala Y: max in alto a destra, metà al centro (interni al box)
	if (ScaleFont)
	{
		PD_Text(Out, Layer + 3, G, X + W - 24.f, Y + 1.f,
			FString::FromInt(MaxVal), *ScaleFont,
			FLinearColor(0.52f, 0.52f, 0.52f, 0.85f));
		if (MaxVal > 1)
			PD_Text(Out, Layer + 3, G, X + W - 24.f, Y + H * 0.5f - 5.f,
				FString::FromInt(MaxVal / 2), *ScaleFont,
				FLinearColor(0.40f, 0.40f, 0.40f, 0.75f));
	}
}

// Barra orizzontale categorica
static void PD_CatBar(
	FSlateWindowElementList& Out, int32 Layer,
	const FGeometry& G, float X, float Y,
	float LabelW, float BarMaxW, float BarH,
	const FString& Label, int32 Count, int32 MaxCount,
	const FLinearColor& Col, const FSlateFontInfo& Font)
{
	PD_Text(Out, Layer, G, X, Y, Label, Font, FLinearColor(0.78f, 0.78f, 0.78f, 1.f));
	const float FillW = (MaxCount > 0) ? BarMaxW * (float)Count / MaxCount : 0.f;
	PD_Box(Out, Layer,     G, X + LabelW, Y, BarMaxW, BarH, FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
	if (FillW > 0.f)
		PD_Box(Out, Layer + 1, G, X + LabelW, Y, FillW, BarH, Col);
	PD_Text(Out, Layer + 2, G, X + LabelW + FillW + 3.f, Y,
		FString::Printf(TEXT("%d"), Count), Font, FLinearColor(0.50f, 0.50f, 0.50f, 1.f));
}

// ──────────────────────────────────────────────────────────────────────

static void FillBucket(TArray<int32>& H, float Val, int32 Bins)
{
	H[FMath::Clamp(FMath::FloorToInt(Val * Bins), 0, Bins - 1)]++;
}

// ──────────────────────────────────────────────────────────────────────
// RefreshData — chiamato ogni 3s su subset NPC spawnati
// ──────────────────────────────────────────────────────────────────────

void UW_populationPanel::RefreshData(U_populationSubsystem* PopSub, const TArray<int32>& SpawnedIds)
{
	if (!PopSub || SpawnedIds.Num() == 0) return;
	const int32 M = SpawnedIds.Num();

	auto InitH = [](TArray<int32>& H, int32 B) { H.Init(0, B); };

	// §3.1 Dual
	InitH(H_MyMuscle, BINS); InitH(H_DsMuscle, BINS);
	InitH(H_MySlim,   BINS); InitH(H_DsSlim,   BINS);
	InitH(H_MyBeauty, BINS); InitH(H_DsBeauty, BINS);
	InitH(H_MyMasc,   BINS); InitH(H_DsMasc,   BINS);
	InitH(H_MyHair,   BINS); InitH(H_DsHair,   BINS);
	InitH(H_MyAge,    BINS); InitH(H_DsAge,    BINS);
	InitH(H_MyBT,     BINS); InitH(H_DsBT,     BINS);

	// §3.1 Single
	InitH(H_Queerness, BINS); InitH(H_Polish,    BINS); InitH(H_BodyDisplay, BINS);
	InitH(H_SexDepth,  BINS); InitH(H_SexExtrem, BINS); InitH(H_SelfEsteem,  BINS);

	// §3.2 Categorical
	Cat_Eth.Init(0, 6);
	Cat_Poz.Init(0, 3);
	TMap<FString, int32> TribeCount;

	// §3.3 Appeal
	InitH(H_PopVis, BINS); InitH(H_PopSex, BINS); InitH(H_PopApp, BINS);

	// §3.5 Balance + accumulatori App-sorted
	InitH(H_Balance, BINS);
	TArray<int32> AppBinOutSum; AppBinOutSum.Init(0, BINS);
	TArray<int32> AppBinInSum;  AppBinInSum.Init(0, BINS);
	TArray<int32> AppBinCnt;    AppBinCnt.Init(0, BINS);

	// Accumulatori per DS/DG (calcolati su subset spawned)
	float SumDs[7] = {}, SumMy[7] = {};

	// ── Loop NPC spawnati ─────────────────────────────────────────
	for (int32 id : SpawnedIds)
	{
		const F_npcProfile& P = PopSub->GetProfile(id);

		// §3.1 Dual
		FillBucket(H_MyMuscle, P.MyMuscle,             BINS);
		FillBucket(H_DsMuscle, P.myDesired_Muscle,      BINS);
		FillBucket(H_MySlim,   P.MySlim,                BINS);
		FillBucket(H_DsSlim,   P.myDesired_Slim,        BINS);
		FillBucket(H_MyBeauty, P.MyBeauty,              BINS);
		FillBucket(H_DsBeauty, P.myDesired_Beauty,      BINS);
		FillBucket(H_MyMasc,   P.MyMasculinity,         BINS);
		FillBucket(H_DsMasc,   P.myDesired_Masculinity, BINS);
		FillBucket(H_MyHair,   P.MyHair,                BINS);
		FillBucket(H_DsHair,   P.myDesired_Hair,        BINS);
		FillBucket(H_MyAge,    P.myAgeNorm,             BINS);
		FillBucket(H_DsAge,    P.myDesired_Age,         BINS);
		FillBucket(H_MyBT,     P.MyBottomTop,           BINS);
		FillBucket(H_DsBT,     P.myDesired_BottomTop,   BINS);

		// §3.1 Single
		FillBucket(H_Queerness,   P.MyQueerness,    BINS);
		FillBucket(H_Polish,      P.MyPolish,       BINS);
		FillBucket(H_BodyDisplay, P.MyBodyDisplay,  BINS);
		FillBucket(H_SexDepth,    P.MySexActDepth,  BINS);
		FillBucket(H_SexExtrem,   P.MySexExtremity, BINS);
		FillBucket(H_SelfEsteem,  P.MySelfEsteem,   BINS);

		// §3.2 Categorical
		Cat_Eth[(int32)P.MyEthnicity]++;
		Cat_Poz[(int32)P.MyPozStatus]++;
		TribeCount.FindOrAdd(P.myTribeTag)++;

		// §3.3 Appeal
		FillBucket(H_PopVis, PopSub->GetPopVisAttraction(id), BINS);
		FillBucket(H_PopSex, PopSub->GetPopSexAttraction(id), BINS);
		FillBucket(H_PopApp, PopSub->GetPopAppAttraction(id), BINS);

		// §3.5 Balance
		const float Bal    = FMath::Clamp(PopSub->GetVisAttractionBalance(id), 0.f, 3.f);
		const int32 BalIdx = FMath::Clamp(FMath::FloorToInt(Bal / 3.f * BINS), 0, BINS - 1);
		H_Balance[BalIdx]++;

		// §3.5 App-sorted
		const float AppScore = PopSub->GetPopAppAttraction(id);
		const int32 AppBin   = FMath::Clamp(FMath::FloorToInt(AppScore * BINS), 0, BINS - 1);
		AppBinOutSum[AppBin] += PopSub->GetOutVisAttractionCount(id);
		AppBinInSum[AppBin]  += PopSub->GetInVisAttractionCount(id);
		AppBinCnt[AppBin]++;

		// DS/DG accumulatori
		SumDs[0] += P.myDesired_Muscle;      SumMy[0] += P.MyMuscle;
		SumDs[1] += P.myDesired_Slim;        SumMy[1] += P.MySlim;
		SumDs[2] += P.myDesired_Beauty;      SumMy[2] += P.MyBeauty;
		SumDs[3] += P.myDesired_Masculinity; SumMy[3] += P.MyMasculinity;
		SumDs[4] += P.myDesired_Hair;        SumMy[4] += P.MyHair;
		SumDs[5] += P.myDesired_Age;         SumMy[5] += P.myAgeNorm;
		SumDs[6] += P.myDesired_BottomTop;   SumMy[6] += P.MyBottomTop;
	}

	// §3.2 Tribe sort desc
	Cat_Tribe.Empty();
	for (auto& Pair : TribeCount)
		Cat_Tribe.Add(TPair<FString, int32>(Pair.Key, Pair.Value));
	Cat_Tribe.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B) {
		return A.Value > B.Value;
	});

	// §3.3 Gini su vis attraction del subset spawned
	{
		TArray<float> VisVals;
		VisVals.Reserve(M);
		for (int32 id : SpawnedIds)
			VisVals.Add(PopSub->GetPopVisAttraction(id));
		VisVals.Sort();

		float SumV = 0.f, WeightedSum = 0.f;
		for (int32 i = 0; i < M; ++i)
		{
			SumV        += VisVals[i];
			WeightedSum += (2.f * (i + 1) - M - 1) * VisVals[i];
		}
		Gini_Vis = (M > 0 && SumV > 0.f)
			? FMath::Clamp(WeightedSum / (M * SumV), 0.f, 1.f)
			: 0.f;
	}

	// §3.4 DS e DG su subset spawned
	for (int32 t = 0; t < 7; ++t)
	{
		DS[t] = SumMy[t] > 0.f ? SumDs[t] / SumMy[t] : 1.f;
		DG[t] = M > 0 ? (SumDs[t] - SumMy[t]) / M : 0.f;
	}

	// §3.4 Pair metrics su subset spawned (O(M²), M≈50 → ~1225 coppie, triviale)
	{
		constexpr float VT = 0.35f; // VIS_STANDARD_THRESHOLD
		constexpr float ST = 0.35f; // SEX_STANDARD_THRESHOLD
		constexpr float AT = 0.35f; // App threshold

		int32 TotalPairs = 0, VisMatched = 0, VisSexMatched = 0;
		int32 AppMatched = 0, BtIncompat = 0, KinkIncompat = 0;

		for (int32 ai = 0; ai < M; ++ai)
		{
			const int32 A = SpawnedIds[ai];
			const F_npcProfile& PA = PopSub->GetProfile(A);

			for (int32 bi = ai + 1; bi < M; ++bi)
			{
				const int32 B = SpawnedIds[bi];
				const F_npcProfile& PB = PopSub->GetProfile(B);

				++TotalPairs;

				const float VisAB = PopSub->GetVisScore(A, B);
				const float VisBA = PopSub->GetVisScore(B, A);
				const bool  bVisMut = (VisAB >= VT && VisBA >= VT);

				if (bVisMut)
				{
					++VisMatched;

					const float SexAB = PopSub->GetSexScore(A, B);
					const float SexBA = PopSub->GetSexScore(B, A);
					if (SexAB >= ST && SexBA >= ST) ++VisSexMatched;

					// BT incompat: entrambi top (>0.6) o entrambi bottom (<0.4)
					const bool bBT =
						(PA.MyBottomTop > 0.6f && PB.MyBottomTop > 0.6f) ||
						(PA.MyBottomTop < 0.4f && PB.MyBottomTop < 0.4f);
					if (bBT) ++BtIncompat;
				}

				const float AppAB = PopSub->GetAppScore(A, B);
				const float AppBA = PopSub->GetAppScore(B, A);
				if (AppAB >= AT && AppBA >= AT) ++AppMatched;

				// Kink incompat: almeno uno estremo e preferenze sessuali distanti
				const bool bKink =
					(PA.MySexExtremity > 0.70f || PB.MySexExtremity > 0.70f) &&
					FMath::Abs(PA.MySexActDepth - PB.MySexActDepth) > 0.40f;
				if (bKink) ++KinkIncompat;
			}
		}

		myVisCompat    = TotalPairs > 0 ? (float)VisMatched      / TotalPairs : 0.f;
		myVisSexCompat = TotalPairs > 0 ? (float)VisSexMatched   / TotalPairs : 0.f;
		myAppCompat    = TotalPairs > 0 ? (float)AppMatched      / TotalPairs : 0.f;
		myBtIncompat   = VisMatched > 0 ? (float)BtIncompat      / VisMatched : 0.f;
		myKinkIncompat = TotalPairs > 0 ? (float)KinkIncompat    / TotalPairs : 0.f;
	}

	// §3.4 Greedy mutual match simulation su subset spawned
	{
		struct FMPair { int32 A; int32 B; float MutVis; };
		TArray<FMPair> Pairs;

		constexpr float VT = 0.35f;
		constexpr float ST = 0.35f;

		for (int32 ai = 0; ai < M; ++ai)
			for (int32 bi = ai + 1; bi < M; ++bi)
			{
				const int32 A = SpawnedIds[ai];
				const int32 B = SpawnedIds[bi];
				const float VisAB = PopSub->GetVisScore(A, B);
				const float VisBA = PopSub->GetVisScore(B, A);
				if (VisAB >= VT && VisBA >= VT)
					Pairs.Add({ A, B, VisAB + VisBA });
			}

		Pairs.Sort([](const FMPair& X, const FMPair& Y) { return X.MutVis > Y.MutVis; });

		TArray<bool> Matched;
		Matched.Init(false, PopSub->GetPopulationSize()); // indexed by NPC id
		myMatch_Sexual  = 0;
		myMatch_VisOnly = 0;

		for (const FMPair& P : Pairs)
		{
			if (Matched[P.A] || Matched[P.B]) continue;

			const float SexAB = PopSub->GetSexScore(P.A, P.B);
			const float SexBA = PopSub->GetSexScore(P.B, P.A);

			if (SexAB >= ST && SexBA >= ST)
				++myMatch_Sexual;
			else
				++myMatch_VisOnly;

			Matched[P.A] = true;
			Matched[P.B] = true;
		}

		int32 MatchedCount = 0;
		for (int32 id : SpawnedIds)
			if (Matched[id]) ++MatchedCount;
		myMatch_None = M - MatchedCount;
	}

	// §3.5 App-sorted averages
	InitH(H_AppSorted_OutVis, BINS);
	InitH(H_AppSorted_InVis,  BINS);

	for (int32 b = 0; b < BINS; ++b)
	{
		if (AppBinCnt[b] > 0)
		{
			H_AppSorted_OutVis[b] = AppBinOutSum[b] / AppBinCnt[b];
			H_AppSorted_InVis[b]  = AppBinInSum[b]  / AppBinCnt[b];
		}
	}

	myPopSize      = M;
	b_myDataLoaded = true;
}

// ──────────────────────────────────────────────────────────────────────
// NativePaint — solo disegno, nessun calcolo
// ──────────────────────────────────────────────────────────────────────

int32 UW_populationPanel::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// Se i dati non sono stati caricati, non disegniamo nulla
	if (!b_myDataLoaded || myPopSize <= 0) return LayerId;

	// Font di base
	const FSlateFontInfo FontLg = FCoreStyle::GetDefaultFontStyle("Regular", 16);
	const FSlateFontInfo FontMed = FCoreStyle::GetDefaultFontStyle("Bold", 12);
	const FSlateFontInfo FontSm = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	const FLinearColor ColDim   (0.60f, 0.60f, 0.60f, 1.f);
	const FLinearColor ColBlue  (0.25f, 0.55f, 0.95f, 1.f);
	const FLinearColor ColOrange(0.95f, 0.55f, 0.15f, 1.f);
	const FLinearColor ColTeal  (0.25f, 0.82f, 0.72f, 1.f);
	const FLinearColor ColPurple(0.65f, 0.30f, 0.90f, 1.f);

	// ======================================================================
	// CALCOLO DINAMICO DELLE DIMENSIONI (RESPONSIVE LAYOUT)
	// ======================================================================
	
	// 1. Leggiamo la larghezza effettiva concessa dallo schermo/ScrollBox
	const float TotalWidth = AllottedGeometry.GetLocalSize().X;
	const float WW = TotalWidth;
	
	// 2. Impostiamo i margini e gli spazi
	const float MG = 40.f;         // Margine sinistro e destro
	const float GAP = 30.f;        // Spazio vuoto tra le colonne
	const float BAR_H = 80.f;      // Altezza fissa degli istogrammi
	const float ROW_H = 14.f;      // Altezza righe di testo
	
	// 3. Calcoliamo la larghezza di ciascuna delle 3 colonne principali
	// (Larghezza totale - margini laterali - i due spazi tra le 3 colonne) diviso 3
	const float COL_W = (TotalWidth - (MG * 2) - (GAP * 2)) / 3.f; 
	
	// 4. Calcoliamo le coordinate X di partenza per ogni colonna
	const float X1 = MG;                       // Colonna di sinistra
	const float X2 = X1 + COL_W + GAP;         // Colonna centrale
	const float X3 = X2 + COL_W + GAP;         // Colonna di destra
	// §3.1 dual histograms: tutta la larghezza utile divisa in 2 metà
	const float COL2_W = (WW - MG * 2.f - GAP) / 2.f;
	const float X2L = MG;                   // metà sinistra: dal margine
	const float X2R = MG + COL2_W + GAP;   // metà destra

	// §3.2 categorical bar dimensions
	const float CAT_LW = 80.f;   // larghezza colonna etichetta
	const float CAT_BW = 200.f;  // larghezza massima barra
	const float CAT_BH = 18.f;   // altezza barra (adattata a FontMed)

	// 5. Variabili specifiche per la sezione §3.5 (Attraction Counts)
	// Qui dividiamo la terza colonna in 3 mini-colonne
	const float SubColW = COL_W / 3.f;
	const float COL3_W = SubColW - 10.f;       // Larghezza di ogni mini-istogramma
	
	const float X3A = X3;                      // Prima mini-colonna
	const float X3B = X3A + SubColW;           // Seconda mini-colonna
	const float X3C = X3B + SubColW;           // Terza mini-colonna

	// Setup iniziale per il disegno
	const int32 L = LayerId;
	float Y = MG; // Y di partenza dall'alt

	// ── Titolo + timer ────────────────────────────────────────────
	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		FString::Printf(TEXT("POPULATION STATISTICS  (N=%d)"), myPopSize),
		FontLg, FLinearColor::White);

	// Timer elapsed (top-right)
	if (const UWorld* World = GetWorld())
	{
		const float Elapsed = World->GetTimeSeconds();
		const int32 Mins    = FMath::FloorToInt(Elapsed / 60.f);
		const int32 Secs    = FMath::FloorToInt(Elapsed) % 60;
		PD_Text(OutDrawElements, L, AllottedGeometry, WW - 52.f, Y,
			FString::Printf(TEXT("%02d:%02d"), Mins, Secs),
			FontMed, FLinearColor(0.50f, 0.50f, 0.50f, 1.f));
	}
	Y += 24.f;

	// ── §3.4 PAIRS COMPATIBILITY (prima sezione) ──────────────────
	PD_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("3.4  PAIRS COMPATIBILITY"), FontMed);

	// Greedy match — barra proporzionale
	{
		const int32 Safe = FMath::Max(myPopSize, 1);
		const float TW   = WW - MG * 2.f;
		const float WSex = TW * (2.f * myMatch_Sexual)  / Safe;
		const float WVis = TW * (2.f * myMatch_VisOnly) / Safe;
		const float WNo  = FMath::Max(TW - WSex - WVis, 0.f);

		PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			TEXT("Greedy match sim  (soglia vis/sex >= 0.35  |  N x N sorted desc)"),
			FontMed, FLinearColor(0.55f, 0.55f, 0.55f, 1.f));
		Y += 18.f;

		PD_Box(OutDrawElements, L, AllottedGeometry,
			MG,                  Y, WSex, 8.f, FLinearColor(0.20f, 0.85f, 0.35f, 1.f));
		PD_Box(OutDrawElements, L, AllottedGeometry,
			MG + WSex,           Y, WVis, 8.f, FLinearColor(0.90f, 0.80f, 0.15f, 1.f));
		PD_Box(OutDrawElements, L, AllottedGeometry,
			MG + WSex + WVis,    Y, WNo,  8.f, FLinearColor(0.85f, 0.35f, 0.15f, 1.f));
		Y += 10.f + GAP * 0.5f;

		const float PctSex = 100.f * 2 * myMatch_Sexual  / Safe;
		const float PctVis = 100.f * 2 * myMatch_VisOnly / Safe;
		const float PctNo  = 100.f * myMatch_None         / Safe;

		PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			FString::Printf(TEXT("Sexual match:  %4d pairs  (%4.1f%% of N)"),
				myMatch_Sexual, PctSex),
			FontMed, FLinearColor(0.25f, 0.92f, 0.42f, 1.f));
		Y += 18.f;
		PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			FString::Printf(TEXT("Visual only:   %4d pairs  (%4.1f%% of N)"),
				myMatch_VisOnly, PctVis),
			FontMed, FLinearColor(0.92f, 0.82f, 0.18f, 1.f));
		Y += 18.f;
		PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			FString::Printf(TEXT("Unmatched:     %4d indiv. (%4.1f%% of N)"),
				myMatch_None, PctNo),
			FontMed, FLinearColor(0.95f, 0.42f, 0.18f, 1.f));
		Y += 18.f + GAP;
	}

	// Metriche scalari
	auto DrawPct = [&](const TCHAR* Label, float Val, const FLinearColor& Col)
	{
		PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			FString::Printf(TEXT("%-38s  %5.1f%%"), Label, Val * 100.f), FontMed, Col);
		Y += 18.f;
	};

	DrawPct(TEXT("Vis compatible pairs"),           myVisCompat,    FLinearColor(0.35f, 0.95f, 0.35f, 1.f));
	DrawPct(TEXT("Vis+Sex compatible"),             myVisSexCompat, FLinearColor(0.30f, 0.80f, 0.30f, 1.f));
	DrawPct(TEXT("App compatible pairs"),           myAppCompat,    FLinearColor(0.35f, 0.75f, 0.95f, 1.f));
	DrawPct(TEXT("BT incompat. (vis-matched)"),     myBtIncompat,   FLinearColor(1.f,   0.50f, 0.20f, 1.f));
	DrawPct(TEXT("Kink incompatibility"),           myKinkIncompat, FLinearColor(1.f,   0.35f, 0.35f, 1.f));

	Y += 5.f;
	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("D/S = demand/supply ratio   DG = desire gap"),
		FontMed, FLinearColor(0.50f, 0.50f, 0.50f, 1.f));
	Y += 18.f;
	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("Trait            D/S       DG"),
		FontMed, FLinearColor(0.62f, 0.62f, 0.62f, 1.f));
	Y += 18.f;

	const TCHAR* DSLabels[7] = {
		TEXT("Muscle"), TEXT("Slim"),  TEXT("Beauty"),
		TEXT("Masc"),   TEXT("Hair"),  TEXT("Age"), TEXT("BT") };

	for (int32 i = 0; i < 7; ++i)
	{
		FLinearColor Col = FLinearColor(0.28f, 0.90f, 0.28f, 1.f);
		if      (DS[i] > 1.1f) Col = FLinearColor(1.f,   0.28f, 0.28f, 1.f);
		else if (DS[i] < 0.9f) Col = FLinearColor(0.28f, 0.48f, 1.f,   1.f);
		PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
			FString::Printf(TEXT("%-14s  %5.2f  %+6.3f"), DSLabels[i], DS[i], DG[i]),
			FontMed, Col);
		Y += 18.f;
	}
	Y += GAP;

	// ── §3.1 Dual histograms ──────────────────────────────────────
	PD_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("3.1  BODY / PRESENCE  — blue=My  orange=Desired"), FontMed);

	auto DrawDual = [&](
		const TCHAR* LL, const TArray<int32>& AL, const TArray<int32>& BL,
		const TCHAR* LR, const TArray<int32>& AR, const TArray<int32>& BR)
	{
		PD_Text(OutDrawElements, L, AllottedGeometry, X2L, Y, FString(LL), FontSm, ColDim);
		PD_Text(OutDrawElements, L, AllottedGeometry, X2R, Y, FString(LR), FontSm, ColDim);
		Y += 11.f;
		PD_Histogram(OutDrawElements, L, AllottedGeometry,
			X2L, Y, COL2_W, BAR_H, AL, ColBlue, &BL, ColOrange, &FontSm);
		PD_Histogram(OutDrawElements, L, AllottedGeometry,
			X2R, Y, COL2_W, BAR_H, AR, ColBlue, &BR, ColOrange, &FontSm);
		Y += BAR_H + GAP;
	};

	DrawDual(TEXT("Muscle"),      H_MyMuscle, H_DsMuscle,
	         TEXT("Slim"),        H_MySlim,   H_DsSlim);
	DrawDual(TEXT("Beauty"),      H_MyBeauty, H_DsBeauty,
	         TEXT("Masculinity"), H_MyMasc,   H_DsMasc);
	DrawDual(TEXT("Hair"),        H_MyHair,   H_DsHair,
	         TEXT("Age (norm.)"), H_MyAge,    H_DsAge);

	PD_Text(OutDrawElements, L, AllottedGeometry, X2L, Y,
		TEXT("Bottom/Top  (0=bottom  1=top)"), FontSm, ColDim);
	Y += 11.f;
	PD_Histogram(OutDrawElements, L, AllottedGeometry,
		X2L, Y, COL2_W, BAR_H, H_MyBT, ColBlue, &H_DsBT, ColOrange, &FontSm);
	Y += BAR_H + GAP * 2.f;

	// ── §3.1 Single histograms ────────────────────────────────────
	PD_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("3.1  SINGLE  — teal"), FontMed);

	auto DrawSingle = [&](
		const TCHAR* LL, const TArray<int32>& AL,
		const TCHAR* LR, const TArray<int32>& AR)
	{
		PD_Text(OutDrawElements, L, AllottedGeometry, X2L, Y, FString(LL), FontSm, ColDim);
		PD_Text(OutDrawElements, L, AllottedGeometry, X2R, Y, FString(LR), FontSm, ColDim);
		Y += 11.f;
		PD_Histogram(OutDrawElements, L, AllottedGeometry,
			X2L, Y, COL2_W, BAR_H, AL, ColTeal, nullptr, FLinearColor::White, &FontSm);
		PD_Histogram(OutDrawElements, L, AllottedGeometry,
			X2R, Y, COL2_W, BAR_H, AR, ColTeal, nullptr, FLinearColor::White, &FontSm);
		Y += BAR_H + GAP;
	};

	DrawSingle(TEXT("Queerness"),     H_Queerness,   TEXT("Polish"),      H_Polish);
	DrawSingle(TEXT("Body Display"),  H_BodyDisplay, TEXT("Sex Depth"),   H_SexDepth);
	DrawSingle(TEXT("Sex Extremity"), H_SexExtrem,   TEXT("Self Esteem"), H_SelfEsteem);
	Y += GAP;

	// ── §3.2 Categorical ──────────────────────────────────────────
	PD_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("3.2  CATEGORICAL"), FontMed);

	const TCHAR* EthLabels[] = {
		TEXT("White"), TEXT("Latino"), TEXT("Black"),
		TEXT("Asian"), TEXT("Mid.East"), TEXT("Mixed") };
	const TCHAR* PozLabels[] = { TEXT("Neg"), TEXT("Poz U=U"), TEXT("Poz") };

	int32 MaxEth = 1;
	for (int32 V : Cat_Eth) MaxEth = FMath::Max(MaxEth, V);
	int32 MaxPoz = 1;
	for (int32 V : Cat_Poz) MaxPoz = FMath::Max(MaxPoz, V);

	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("Ethnicity"), FontMed, FLinearColor(0.60f, 0.60f, 0.60f, 1.f));
	Y += 16.f;
	for (int32 i = 0; i < 6; ++i)
	{
		PD_CatBar(OutDrawElements, L, AllottedGeometry,
			MG, Y, CAT_LW, CAT_BW, CAT_BH,
			FString(EthLabels[i]), Cat_Eth[i], MaxEth,
			FLinearColor(0.28f, 0.52f, 0.78f, 1.f), FontMed);
		Y += CAT_BH + 4.f;
	}

	Y += 8.f;
	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("Poz Status"), FontMed, FLinearColor(0.60f, 0.60f, 0.60f, 1.f));
	Y += 16.f;
	for (int32 i = 0; i < 3; ++i)
	{
		PD_CatBar(OutDrawElements, L, AllottedGeometry,
			MG, Y, CAT_LW, CAT_BW, CAT_BH,
			FString(PozLabels[i]), Cat_Poz[i], MaxPoz,
			FLinearColor(0.70f, 0.30f, 0.30f, 1.f), FontMed);
		Y += CAT_BH + 4.f;
	}

	Y += 8.f;
	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("Tribe (top 8)"), FontMed, FLinearColor(0.60f, 0.60f, 0.60f, 1.f));
	Y += 16.f;
	const int32 TribeShow = FMath::Min(Cat_Tribe.Num(), 8);
	const int32 MaxTribe  = TribeShow > 0 ? Cat_Tribe[0].Value : 1;
	for (int32 i = 0; i < TribeShow; ++i)
	{
		PD_CatBar(OutDrawElements, L, AllottedGeometry,
			MG, Y, CAT_LW, CAT_BW, CAT_BH,
			Cat_Tribe[i].Key, Cat_Tribe[i].Value, MaxTribe,
			FLinearColor(0.32f, 0.65f, 0.32f, 1.f), FontMed);
		Y += CAT_BH + 4.f;
	}
	Y += GAP;

	// ── §3.3 Appeal ───────────────────────────────────────────────
	PD_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("3.3  APPEAL  — purple"), FontMed);

	PD_Text(OutDrawElements, L, AllottedGeometry, X3A, Y,
		TEXT("Pop Vis Attr."), FontSm, ColDim);
	PD_Text(OutDrawElements, L, AllottedGeometry, X3B, Y,
		TEXT("Pop Sex Attr."), FontSm, ColDim);
	PD_Text(OutDrawElements, L, AllottedGeometry, X3C, Y,
		TEXT("Pop App Attr."), FontSm, ColDim);
	Y += 11.f;
	PD_Histogram(OutDrawElements, L, AllottedGeometry,
		X3A, Y, COL3_W, BAR_H, H_PopVis, ColPurple, nullptr, FLinearColor::White, &FontSm);
	PD_Histogram(OutDrawElements, L, AllottedGeometry,
		X3B, Y, COL3_W, BAR_H, H_PopSex, ColPurple, nullptr, FLinearColor::White, &FontSm);
	PD_Histogram(OutDrawElements, L, AllottedGeometry,
		X3C, Y, COL3_W, BAR_H, H_PopApp, ColPurple, nullptr, FLinearColor::White, &FontSm);
	Y += BAR_H + GAP;

	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		FString::Printf(TEXT("Vis Gini polarization: %.3f   (0=equal, 1=concentrated)"), Gini_Vis),
		FontSm, FLinearColor(0.90f, 0.90f, 0.42f, 1.f));
	Y += ROW_H + GAP;

	// ── §3.5 Attraction counts — App-sorted ───────────────────────
	PD_SectionHeader(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("3.5  ATTRACTION COUNTS  (X = App desirability, low  -->  high)"), FontMed);

	PD_Text(OutDrawElements, L, AllottedGeometry, X3A, Y,
		TEXT("Out-Vis / App rank"), FontSm, ColDim);
	PD_Text(OutDrawElements, L, AllottedGeometry, X3B, Y,
		TEXT("In-Vis / App rank"),  FontSm, ColDim);
	PD_Text(OutDrawElements, L, AllottedGeometry, X3C, Y,
		TEXT("Balance [0-3]"),      FontSm, ColDim);
	Y += 11.f;

	PD_Histogram(OutDrawElements, L, AllottedGeometry,
		X3A, Y, COL3_W, BAR_H, H_AppSorted_OutVis,
		FLinearColor(0.20f, 0.78f, 0.38f), nullptr, FLinearColor::White, &FontSm);
	PD_Histogram(OutDrawElements, L, AllottedGeometry,
		X3B, Y, COL3_W, BAR_H, H_AppSorted_InVis,
		FLinearColor(0.38f, 0.88f, 0.48f), nullptr, FLinearColor::White, &FontSm);
	PD_Histogram(OutDrawElements, L, AllottedGeometry,
		X3C, Y, COL3_W, BAR_H, H_Balance,
		FLinearColor(0.90f, 0.90f, 0.28f), nullptr, FLinearColor::White, &FontSm);
	Y += BAR_H + GAP;

	PD_Text(OutDrawElements, L, AllottedGeometry, MG, Y,
		TEXT("Y = avg abs. attraction count per App-score bin  (n = scale top-right)"),
		FontSm, FLinearColor(0.42f, 0.42f, 0.42f, 1.f));
	Y += ROW_H + MG;

	return L + 3;
}
