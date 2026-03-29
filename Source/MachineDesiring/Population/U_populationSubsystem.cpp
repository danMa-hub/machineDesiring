// Fill out your copyright notice in the Description page of Project Settings.

#include "U_populationSubsystem.h"
#include "Engine/DataTable.h"

// ================================================================
// STRUTTURE DATI STATICHE
// Usano array C-style perché le costanti statiche in UE5
// non supportano TMap/TArray con initializer list {}.
// ================================================================

// ── Coefficienti assortativi ─────────────────────────────────────
// Fonte: Luo 2017 (r_self), Bruch & Newman 2018 (r_ideal)
struct FAssortEntry
{
	const TCHAR* Var;
	float        r_self;
	float        r_ideal;
	float        r_pop;
	float        noise_sd;
};

static const FAssortEntry ASSORT_COEFFS[] =
{
	{ TEXT("Muscle"),      0.25f,  0.35f, 0.40f, 0.12f },
	{ TEXT("Slim"),        0.25f,  0.35f, 0.40f, 0.12f },
	{ TEXT("Beauty"),      0.20f,  0.40f, 0.40f, 0.10f },
	{ TEXT("Masculinity"), 0.45f,  0.20f, 0.35f, 0.12f },  // Bailey 1997
	{ TEXT("Height"),      0.30f,  0.20f, 0.50f, 0.10f },
	{ TEXT("Hair"),        0.15f,  0.00f, 0.85f, 0.28f },  // Muscarella 2002
	{ TEXT("Age"),         0.70f,  0.20f, 0.10f, 0.06f },  // Antfolk 2017
	{ TEXT("BottomTop"),  -0.90f,  0.00f, 0.10f, 0.12f },  // complementare, Moskowitz 2008
	{ TEXT("SubDom"),     -0.85f,  0.00f, 0.15f, 0.12f },  // complementare
	{ TEXT("KinkLevel"),   0.75f,  0.00f, 0.25f, 0.10f },  // speculare
};
static const int32 ASSORT_COUNT = UE_ARRAY_COUNT(ASSORT_COEFFS);

// ── Ideale culturale ─────────────────────────────────────────────
struct FFloatEntry { const TCHAR* Var; float Value; };

static const FFloatEntry CULTURAL_IDEAL[] =
{
	{ TEXT("Muscle"),      0.75f  },
	{ TEXT("Slim"),        0.70f  },
	{ TEXT("Beauty"),      0.80f  },
	{ TEXT("Masculinity"), 0.75f  },
	{ TEXT("Height"),      0.65f  },
	{ TEXT("Age"),         0.122f },  // (28-18)/82, Silverthorne & Quinsey 2000
};
static const int32 IDEAL_COUNT = UE_ARRAY_COUNT(CULTURAL_IDEAL);

static const FFloatEntry POP_MEAN[] =
{
	{ TEXT("Muscle"),      0.30f  },
	{ TEXT("Slim"),        0.50f  },
	{ TEXT("Beauty"),      0.45f  },
	{ TEXT("Hair"),        0.46f  },
	{ TEXT("Masculinity"), 0.58f  },
	{ TEXT("Height"),      0.50f  },
	{ TEXT("Age"),         0.249f },  // (38.4-18)/82
};
static const int32 POPMEAN_COUNT = UE_ARRAY_COUNT(POP_MEAN);

// ── Pesi base visuali ────────────────────────────────────────────
// Fonte: Swami & Tovée 2008, Levesque 2006, Bailey 1997, Martins 2008
static const FFloatEntry BASE_WEIGHTS_VISUAL[] =
{
	{ TEXT("Muscle"),      0.28f },
	{ TEXT("Beauty"),      0.23f },
	{ TEXT("Slim"),        0.15f },
	{ TEXT("Masculinity"), 0.15f },
	{ TEXT("Age"),         0.07f },
	{ TEXT("Hair"),        0.05f },
	{ TEXT("Height"),      0.04f },
};
static const int32 VIS_WEIGHTS_COUNT = UE_ARRAY_COUNT(BASE_WEIGHTS_VISUAL);

// ── Pesi base sessuali ───────────────────────────────────────────
// Fonte: Moskowitz 2008, Bailey 1997, Parsons 2010, GMFA 2017
static const FFloatEntry BASE_WEIGHTS_SEXUAL[] =
{
	{ TEXT("BottomTop"), 0.40f },
	{ TEXT("SubDom"),    0.28f },
	{ TEXT("KinkLevel"), 0.17f },
	{ TEXT("PenisSize"), 0.15f },
};
static const int32 SEX_WEIGHTS_COUNT = UE_ARRAY_COUNT(BASE_WEIGHTS_SEXUAL);

// ── Pesi base single-pass (app) ──────────────────────────────────
// Normalizzazione unica: pesi raw / 1.97. BottomTop domina (Moskowitz 2008)
static const FFloatEntry BASE_WEIGHTS_APP[] =
{
	{ TEXT("BottomTop"),   0.203f },
	{ TEXT("Muscle"),      0.142f },
	{ TEXT("SubDom"),      0.142f },
	{ TEXT("Beauty"),      0.117f },
	{ TEXT("KinkLevel"),   0.086f },
	{ TEXT("Slim"),        0.076f },
	{ TEXT("Masculinity"), 0.076f },
	{ TEXT("PenisSize"),   0.076f },
	{ TEXT("Age"),         0.036f },
	{ TEXT("Hair"),        0.025f },
	{ TEXT("Height"),      0.020f },
};
static const int32 SIN_WEIGHTS_COUNT = UE_ARRAY_COUNT(BASE_WEIGHTS_APP);

// ── Kink types ───────────────────────────────────────────────────
// Fonte: Parsons et al. 2010 (N=1214 MSM)
struct FKinkDef  { float base, kl_min, kl_w, se_w; };
struct FKinkEntry { const TCHAR* Name; FKinkDef Def; };

static const FKinkEntry KINK_TYPES_DEF[] =
{
	{ TEXT("GroupSex"),    { 0.40f, 0.00f, 0.20f, 0.18f } },
	{ TEXT("ExhibVoyeur"), { 0.25f, 0.00f, 0.18f, 0.22f } },
	{ TEXT("LightBDSM"),   { 0.15f, 0.05f, 0.32f, 0.15f } },
	{ TEXT("Watersports"), { 0.10f, 0.08f, 0.30f, 0.22f } },
	{ TEXT("SM_Pain"),     { 0.02f, 0.12f, 0.28f, 0.32f } },
	{ TEXT("Fisting"),     { 0.02f, 0.12f, 0.15f, 0.40f } },
	{ TEXT("PupPlay"),     { 0.02f, 0.18f, 0.22f, 0.12f } },
	{ TEXT("Chemsex"),     { 0.06f, 0.00f, 0.08f, 0.18f } },
	{ TEXT("GearFetish"),  { 0.03f, 0.12f, 0.28f, 0.12f } },
	{ TEXT("Roleplay"),    { 0.08f, 0.00f, 0.18f, 0.10f } },
	{ TEXT("EdgePlay"),    { 0.00f, 0.42f, 0.12f, 0.42f } },
};
static const int32 KINK_COUNT = UE_ARRAY_COUNT(KINK_TYPES_DEF);

// ── Tribe prototypes ─────────────────────────────────────────────
// Fonte: Moskowitz 2013, Grindr tribes 2013, Tran 2020
// var: sl=Slim, h=Hair, m=Muscle, ma=Masculinity, po=Polish, age=AgeNorm
struct FTribeProto { const TCHAR* Var; float Target; float Weight; };
struct FTribeEntry { const TCHAR* Name; FTribeProto Protos[6]; int32 Count; };

static const FTribeEntry TRIBE_PROTOTYPES[] =
{
	{ TEXT("Bear"),       { {TEXT("sl"),0.35f,0.292f},{TEXT("h"),0.72f,0.292f},{TEXT("ma"),0.70f,0.083f},{TEXT("age"),0.268f,0.067f} }, 4 },
	{ TEXT("Cub"),        { {TEXT("sl"),0.30f,0.353f},{TEXT("h"),0.68f,0.353f},{TEXT("age"),0.073f,0.294f} }, 3 },
	{ TEXT("Otter"),      { {TEXT("sl"),0.65f,0.312f},{TEXT("h"),0.65f,0.375f},{TEXT("age"),0.110f,0.150f},{TEXT("m"),0.28f,0.100f} }, 4 },
	{ TEXT("Wolf"),       { {TEXT("sl"),0.60f,0.115f},{TEXT("h"),0.68f,0.192f},{TEXT("m"),0.55f,0.115f},{TEXT("ma"),0.75f,0.192f},{TEXT("age"),0.244f,0.062f} }, 5 },
	{ TEXT("Twink"),      { {TEXT("sl"),0.78f,0.278f},{TEXT("h"),0.12f,0.278f},{TEXT("m"),0.18f,0.167f},{TEXT("age"),0.037f,0.278f},{TEXT("ma"),0.38f,0.111f} }, 5 },
	{ TEXT("Twunk"),      { {TEXT("sl"),0.72f,0.267f},{TEXT("h"),0.18f,0.200f},{TEXT("m"),0.55f,0.333f},{TEXT("age"),0.061f,0.267f} }, 4 },
	{ TEXT("Jock"),       { {TEXT("sl"),0.70f,0.200f},{TEXT("m"),0.68f,0.300f},{TEXT("h"),0.20f,0.200f},{TEXT("age"),0.110f,0.150f},{TEXT("ma"),0.65f,0.100f} }, 5 },
	{ TEXT("Muscle"),     { {TEXT("sl"),0.75f,0.357f},{TEXT("m"),0.82f,0.500f},{TEXT("age"),0.146f,0.114f} }, 3 },
	{ TEXT("MuscleBear"), { {TEXT("m"),0.68f,0.333f},{TEXT("h"),0.65f,0.333f},{TEXT("sl"),0.35f,0.333f} }, 3 },
	{ TEXT("Chub"),       { {TEXT("sl"),0.12f,0.833f},{TEXT("age"),0.268f,0.148f} }, 2 },
	{ TEXT("Daddy"),      { {TEXT("age"),0.415f,0.163f},{TEXT("ma"),0.73f,0.408f},{TEXT("sl"),0.45f,0.109f} }, 3 },
	{ TEXT("CleanCut"),   { {TEXT("po"),0.78f,0.535f},{TEXT("h"),0.18f,0.275f},{TEXT("sl"),0.60f,0.153f} }, 3 },
	{ TEXT("Rugged"),     { {TEXT("po"),0.13f,0.667f},{TEXT("h"),0.60f,0.333f} }, 2 },
	{ TEXT("Femboy"),     { {TEXT("ma"),0.15f,0.750f},{TEXT("h"),0.15f,0.250f} }, 2 },
};
static const int32 TRIBE_COUNT = UE_ARRAY_COUNT(TRIBE_PROTOTYPES);
static const float TRIBE_MIN_SCORE = 0.32f;

// ── Lookup helpers per array statici ────────────────────────────

static float GetIdeal(const TCHAR* Var)
{
	for (int32 i = 0; i < IDEAL_COUNT; i++)
		if (FCString::Strcmp(CULTURAL_IDEAL[i].Var, Var) == 0)
			return CULTURAL_IDEAL[i].Value;
	return 0.5f;
}

static float GetPopMean(const TCHAR* Var)
{
	for (int32 i = 0; i < POPMEAN_COUNT; i++)
		if (FCString::Strcmp(POP_MEAN[i].Var, Var) == 0)
			return POP_MEAN[i].Value;
	return 0.5f;
}

static float GetEthHairMod(E_npcEthnicity Eth)
{
	switch (Eth)
	{
	case E_npcEthnicity::White:         return  0.05f;
	case E_npcEthnicity::Latino:        return  0.05f;
	case E_npcEthnicity::Black:         return -0.05f;
	case E_npcEthnicity::Asian:         return -0.20f;
	case E_npcEthnicity::MiddleEastern: return  0.15f;
	default:                           return  0.00f;
	}
}

// ================================================================
// Initialize / RegeneratePopulation
// ================================================================

void U_populationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GeneratePopulation();
}

void U_populationSubsystem::RegeneratePopulation(int32 NewSeed)
{
	PopulationSeed = NewSeed;
	GeneratePopulation();
}

void U_populationSubsystem::GeneratePopulation()
{
	RNG.Initialize(PopulationSeed);
	Profiles.SetNum(PopulationSize);

	for (int32 i = 0; i < PopulationSize; i++)
	{
		GenerateProfile(i, Profiles[i]);
		GeneratePreferences(Profiles[i]);
	}

	// Pre-calcola tutte le attrazioni — runtime usa solo getter O(1)
	BuildAttractionMatrices();
	BuildMeshScaleCache();
	BuildPopulationMetrics();

	if (InspectionTable)
	{
		SyncToDataTable();
	}
	UE_LOG(LogTemp, Warning, TEXT("NPCPopulation generata: %d profili, seed=%d — matrici OK"),
	       Profiles.Num(), PopulationSeed);
}

// ================================================================
// PASSATA 1-2-3 — GenerateProfile
// ================================================================

void U_populationSubsystem::GenerateProfile(int32 Id, F_npcProfile& P)
{
	P.NpcId = Id;

	// ── PASSATA 1 — Indipendenti ─────────────────────────────────

	P.MyAge        = FMath::Clamp(RandGauss(38.f, 15.7f), 18.f, 100.f);
	P.MyHeight     = RandGauss(177.f, 7.f);
	P.MyEthnicity  = RandEthnicity();
	P.MyPolish     = RandGaussClamp(0.44f, 0.18f);
	P.MyQueerness  = RandBimodal(0.20f, 0.10f, 0.55f, 0.72f, 0.12f);
	P.MyKinkLevel  = Clamp01(RandExpInverse(0.18f, 0.33f));
	P.MyOpenClosed = RandGaussClamp(0.55f, 0.22f);
	P.MyRelDepth   = RandGaussClamp(0.50f, 0.22f);
	// Fonte: Veale et al. 2015 — biologico, indipendente dal ruolo
	P.MyPenisSize  = RandGaussClamp(0.50f, 0.15f);

	// basi intermedie (non memorizzate nel profilo)
	const float HairBase    = RandGauss(0.46f, 0.18f);
	const float BeautyBase  = RandGauss(0.45f, 0.15f);
	const float BodyModBase = RandExpInverse(0.18f, 0.33f);
	const float ExhibBase   = RandExpInverse(0.22f, 0.245f);

	// ── PASSATA 2 — Condizionate ─────────────────────────────────

	const float an = (P.MyAge    - 18.f) / 82.f;
	const float hn = Clamp01((P.MyHeight - 160.f) / 35.f);
	P.myAgeNorm    = an;
	P.myHeightNorm = hn;

	// Hair — base + invecchiamento + modificatore etnia
	P.MyHair = Clamp01(HairBase + an * 0.10f + GetEthHairMod(P.MyEthnicity));

	// BodyMod — decrescita con età, boost queerness
	P.MyBodyMod = Clamp01(BodyModBase - FMath::Max(0.f, (an - 0.30f)) * 0.12f
	                      + P.MyQueerness * 0.10f);

	// BottomTop — trimodale B28/V50/T22 + condizionamento età/altezza/pene
	// Fonte: Moskowitz & Hart 2011 (pene→ruolo), CDC/Yale Law 2014 (distribuzione)
	{
		const float Roll = RNG.FRand();
		float bt;
		if      (Roll < 0.28f) bt = RandGaussClamp(0.10f, 0.08f);
		else if (Roll < 0.78f) bt = RandGaussClamp(0.50f, 0.10f);
		else                   bt = RandGaussClamp(0.90f, 0.08f);
		bt += (an - 0.33f) * 0.10f;
		bt += (hn - 0.50f) * 0.10f;
		bt += (P.MyPenisSize - 0.50f) * 0.10f;
		P.MyBottomTop = Clamp01(bt);
	}

	// Muscle — piecewise con età (Doherty 2003: sarcopenia)
	{
		const float MuscleBase = RandGauss(0.30f, 0.15f);
		float AgePenalty;
		if      (an < 0.21f) AgePenalty = -(0.21f - an) * 0.08f;   // < 35: bonus
		else if (an < 0.51f) AgePenalty =  (an - 0.21f) * 0.15f;   // 35-60: moderato
		else                 AgePenalty =  (0.30f * 0.15f) + (an - 0.51f) * 0.28f; // > 60: accelerato
		P.MyMuscle = Clamp01(MuscleBase - AgePenalty);
	}

	// Slim — NHANES calibrato + bonus muscle nonlineare
	// Fonte: NHANES 1999-2004, r(ASMI,%fat)=-0.761
	{
		const float SlimBase        = RandGauss(0.48f, 0.22f);
		const float MuscleSlimBonus = FMath::Max(0.f, P.MyMuscle - 0.30f) * 0.35f;
		P.MySlim = Clamp01(SlimBase - an * 0.15f + MuscleSlimBonus);
	}

	// Beauty — decay forte (Schope 2005, Rhodes 2006)
	P.MyBeauty = Clamp01(BeautyBase - an * 0.20f);

	// Masculinity — hair, queerness, height
	P.MyMasculinity = Clamp01(RandGauss(0.62f, 0.18f)
	                          + P.MyHair      * 0.15f
	                          - P.MyQueerness * 0.20f
	                          + (hn - 0.50f)  * 0.08f);

	// BodyDisplay — body display per LOD vestiti UE5
	P.MyBodyDisplay = Clamp01(ExhibBase + P.MyMuscle * 0.10f);

	// SexExtremity — condiziona KinkTypes
	P.MySexExtremity = Clamp01(RandGauss(0.32f, 0.18f) + P.MyKinkLevel * 0.25f);

	// SexActDepth — Richters 2008 (fuori dal match)
	P.MySexActDepth = Clamp01(RandGauss(0.38f, 0.20f) + P.MySexExtremity * 0.15f);

	// SubDom — Holvoet 2017: Top/Bottom e Dom/Sub sono dimensioni indipendenti
	P.MySubDom = Clamp01(RandGauss(0.44f, 0.20f) + (P.MyBottomTop - 0.50f) * 0.10f);

	// PozStatus — age-dependent (CDC 2022)
	// Fix v2.3: af modula solo hiv_rate, None è il complemento
	{
		const float af = (P.MyAge < 25.f) ? 0.40f
		               : (P.MyAge < 35.f) ? 1.00f
		               : (P.MyAge < 50.f) ? 1.30f : 0.70f;
		const float HivRate    = FMath::Min(0.40f, 0.18f * af);
		const float UndetFrac  = 0.67f;
		P.MyPozStatus = RandPozStatus(HivRate, UndetFrac);
	}

	// SafetyPractice — Fix v2.3: PrEP solo HIV-negativi
	P.MySafetyPractice = RandSafetyPractice(P.MyPozStatus);

	// ── PASSATA 3 — Derivate ─────────────────────────────────────

	// SelfEsteem — sociometro (Morrison 2004, Peplau 2009, Grov 2010)
	// Base 0.21 calibrata su target gay ~0.48
	{
		const float SeObj = P.MyBeauty      * 0.20f
		                  + P.MyMuscle      * 0.15f
		                  + P.MySlim        * 0.10f
		                  + P.MyMasculinity * 0.08f
		                  + P.MyPenisSize   * 0.12f
		                  - an              * 0.10f;
		P.MySelfEsteem = Clamp01(0.21f + SeObj + RandGauss(0.f, 0.15f));
	}

	// TribeTag — prototype scoring euclideo (SOLO LOD/debug)
	P.myTribeTag = ClassifyTribe(P);

	// KinkTypes — flag multipli (Parsons 2010, Dean 2009)
	AssignKinkTypes(P);
}

// ================================================================
// GeneratePreferences
// ================================================================

void U_populationSubsystem::GeneratePreferences(F_npcProfile& P)
{
	const float an = P.myAgeNorm;
	const float hn = P.myHeightNorm;

	// Helper: valore del profilo per variabile
	auto GetMyVal = [&](const TCHAR* Var) -> float
	{
		if (FCString::Strcmp(Var, TEXT("Muscle"))      == 0) return P.MyMuscle;
		if (FCString::Strcmp(Var, TEXT("Slim"))        == 0) return P.MySlim;
		if (FCString::Strcmp(Var, TEXT("Beauty"))      == 0) return P.MyBeauty;
		if (FCString::Strcmp(Var, TEXT("Hair"))        == 0) return P.MyHair;
		if (FCString::Strcmp(Var, TEXT("Masculinity")) == 0) return P.MyMasculinity;
		if (FCString::Strcmp(Var, TEXT("Height"))      == 0) return hn;
		if (FCString::Strcmp(Var, TEXT("Age"))         == 0) return an;
		if (FCString::Strcmp(Var, TEXT("BottomTop"))   == 0) return P.MyBottomTop;
		if (FCString::Strcmp(Var, TEXT("SubDom"))      == 0) return P.MySubDom;
		if (FCString::Strcmp(Var, TEXT("KinkLevel"))   == 0) return P.MyKinkLevel;
		return 0.5f;
	};

	// Helper: scrivi desired nel profilo
	auto SetDesired = [&](const TCHAR* Var, float Val)
	{
		Val = Clamp01(Val);
		if (FCString::Strcmp(Var, TEXT("Muscle"))      == 0) P.myDesired_Muscle      = Val;
		if (FCString::Strcmp(Var, TEXT("Slim"))        == 0) P.myDesired_Slim        = Val;
		if (FCString::Strcmp(Var, TEXT("Beauty"))      == 0) P.myDesired_Beauty      = Val;
		if (FCString::Strcmp(Var, TEXT("Hair"))        == 0) P.myDesired_Hair        = Val;
		if (FCString::Strcmp(Var, TEXT("Masculinity")) == 0) P.myDesired_Masculinity = Val;
		if (FCString::Strcmp(Var, TEXT("Height"))      == 0) P.myDesired_Height      = Val;
		if (FCString::Strcmp(Var, TEXT("Age"))         == 0) P.myDesired_Age         = Val;
		if (FCString::Strcmp(Var, TEXT("BottomTop"))   == 0) P.myDesired_BottomTop   = Val;
		if (FCString::Strcmp(Var, TEXT("SubDom"))      == 0) P.myDesired_SubDom      = Val;
		if (FCString::Strcmp(Var, TEXT("KinkLevel"))   == 0) P.myDesired_KinkLevel   = Val;
	};

	// ── myDesired_ da ASSORT_COEFFS ─────────────────────────────
	for (int32 i = 0; i < ASSORT_COUNT; i++)
	{
		const FAssortEntry& C  = ASSORT_COEFFS[i];
		const float MyVal      = GetMyVal(C.Var);
		const float Ideal      = GetIdeal(C.Var);
		const float PopM       = GetPopMean(C.Var);

		float Desired;
		if (C.r_self < 0.f)
		{
			// Complementare — desidero l'opposto di me
			Desired = FMath::Abs(C.r_self) * (1.f - MyVal)
			        + (1.f - FMath::Abs(C.r_self)) * PopM;
		}
		else if (C.r_ideal > 0.f)
		{
			// Aspirazionale — ancoraggio sé + pull ideale + pull media
			Desired = C.r_self * MyVal + C.r_ideal * Ideal + C.r_pop * PopM;
		}
		else
		{
			// Assortativo puro — solo sé + media
			Desired = C.r_self * MyVal + (1.f - C.r_self) * PopM;
		}

		Desired += RandGauss(0.f, C.noise_sd);
		SetDesired(C.Var, Desired);
	}

	// myDesired_PenisSize — BT-condizionale, fuori da ASSORT_COEFFS
	// Fonte: GMFA 2017 — bottom vuole top dotato
	P.myDesired_PenisSize = Clamp01(
		0.50f + (1.f - P.MyBottomTop) * 0.25f + RandGauss(0.f, 0.12f));

	// ── Modulatori condivisi ─────────────────────────────────────
	// Fonte: Li 2002 (BT necessità), Parsons 2010 (kink), GMFA 2017 (pene/bottom)
	const float BtExtremity = FMath::Abs(P.MyBottomTop - 0.5f) * 2.f;
	const float Bottomness  = 1.f - P.MyBottomTop;

	// ── myWeightsVisual ──────────────────────────────────────────
	// FIX-7 v2.3: Masculinity e Hair noise 40%, altri 30% (Bailey 1997, Martins 2008)
	float WVis[7];
	float TotVis = 0.f;
	for (int32 i = 0; i < VIS_WEIGHTS_COUNT; i++)
	{
		const float Noise = (FCString::Strcmp(BASE_WEIGHTS_VISUAL[i].Var, TEXT("Masculinity")) == 0
		                  || FCString::Strcmp(BASE_WEIGHTS_VISUAL[i].Var, TEXT("Hair"))         == 0)
		                  ? 0.40f : 0.30f;
		WVis[i]  = FMath::Max(0.01f, BASE_WEIGHTS_VISUAL[i].Value
		         + RandGauss(0.f, BASE_WEIGHTS_VISUAL[i].Value * Noise));
		TotVis  += WVis[i];
	}
	P.wVis_Muscle      = WVis[0] / TotVis;
	P.wVis_Beauty      = WVis[1] / TotVis;
	P.wVis_Slim        = WVis[2] / TotVis;
	P.wVis_Masculinity = WVis[3] / TotVis;
	P.wVis_Age         = WVis[4] / TotVis;
	P.wVis_Hair        = WVis[5] / TotVis;
	P.wVis_Height      = WVis[6] / TotVis;

	// ── myWeightsSexual ──────────────────────────────────────────
	float WSex[4];
	float TotSex = 0.f;
	for (int32 i = 0; i < SEX_WEIGHTS_COUNT; i++)
	{
		WSex[i]  = FMath::Max(0.01f, BASE_WEIGHTS_SEXUAL[i].Value
		         + RandGauss(0.f, BASE_WEIGHTS_SEXUAL[i].Value * 0.30f));
		TotSex  += WSex[i];
	}
	// indice 0=BottomTop, 1=SubDom, 2=KinkLevel, 3=PenisSize
	WSex[0] *= (1.f + BtExtremity    * 0.60f);
	WSex[2] *= (1.f + P.MyKinkLevel  * 0.80f);
	WSex[3] *= (1.f + Bottomness     * 0.80f);
	TotSex = WSex[0] + WSex[1] + WSex[2] + WSex[3];
	P.wSex_BottomTop = WSex[0] / TotSex;
	P.wSex_SubDom    = WSex[1] / TotSex;
	P.wSex_KinkLevel = WSex[2] / TotSex;
	P.wSex_PenisSize = WSex[3] / TotSex;

	// ── myWeightsSingle ──────────────────────────────────────────
	// ordine array: BT(0) Muscle(1) SubDom(2) Beauty(3) KL(4) Slim(5) Masc(6) PS(7) Age(8) Hair(9) Height(10)
	float WSin[11];
	float TotSin = 0.f;
	for (int32 i = 0; i < SIN_WEIGHTS_COUNT; i++)
	{
		const float Noise = (FCString::Strcmp(BASE_WEIGHTS_APP[i].Var, TEXT("Masculinity")) == 0
		                  || FCString::Strcmp(BASE_WEIGHTS_APP[i].Var, TEXT("Hair"))         == 0)
		                  ? 0.40f : 0.30f;
		WSin[i]  = FMath::Max(0.01f, BASE_WEIGHTS_APP[i].Value
		         + RandGauss(0.f, BASE_WEIGHTS_APP[i].Value * Noise));
		TotSin  += WSin[i];
	}
	WSin[0] *= (1.f + BtExtremity    * 0.60f);  // BottomTop
	WSin[4] *= (1.f + P.MyKinkLevel  * 0.80f);  // KinkLevel
	WSin[7] *= (1.f + Bottomness     * 0.80f);  // PenisSize
	TotSin = 0.f;
	for (int32 i = 0; i < SIN_WEIGHTS_COUNT; i++) TotSin += WSin[i];
	P.wSin_BottomTop   = WSin[0]  / TotSin;
	P.wSin_Muscle      = WSin[1]  / TotSin;
	P.wSin_SubDom      = WSin[2]  / TotSin;
	P.wSin_Beauty      = WSin[3]  / TotSin;
	P.wSin_KinkLevel   = WSin[4]  / TotSin;
	P.wSin_Slim        = WSin[5]  / TotSin;
	P.wSin_Masculinity = WSin[6]  / TotSin;
	P.wSin_PenisSize   = WSin[7]  / TotSin;
	P.wSin_Age         = WSin[8]  / TotSin;
	P.wSin_Hair        = WSin[9]  / TotSin;
	P.wSin_Height      = WSin[10] / TotSin;

	// Bias osservatore — generato una volta per persona (non per coppia)
	// Fonte: Callander 2015 (etnia), Smit 2012 (PozStatus)
	P.myEthBias = FMath::Max(0.f, RandGauss(1.f, 0.40f));
	P.myPozBias = FMath::Max(0.f, RandGauss(1.f, 0.50f));
}

// ================================================================
// RNG HELPERS
// ================================================================

float U_populationSubsystem::RandGauss(float Mean, float SD)
{
	// Box-Muller con FRandomStream (deterministico con seed)
	const float U1 = FMath::Max(1e-6f, RNG.FRand());
	const float U2 = RNG.FRand();
	return Mean + SD * FMath::Sqrt(-2.f * FMath::Loge(U1))
	                 * FMath::Cos(2.f * PI * U2);
}

float U_populationSubsystem::RandGaussClamp(float Mean, float SD, float Lo, float Hi)
{
	return FMath::Clamp(RandGauss(Mean, SD), Lo, Hi);
}

float U_populationSubsystem::RandExpInverse(float Mean, float NoiseFrac)
{
	// Equivalente di: clamp(exponential(mean) + normal(0, mean*NoiseFrac))
	// Inverse CDF per distribuzione esponenziale
	const float U = FMath::Max(1e-6f, RNG.FRand());
	const float Exp = -Mean * FMath::Loge(U);
	return Clamp01(Exp + RandGauss(0.f, Mean * NoiseFrac));
}

float U_populationSubsystem::RandBimodal(float M1, float S1, float W1, float M2, float S2)
{
	return Clamp01(RNG.FRand() < W1 ? RandGauss(M1, S1) : RandGauss(M2, S2));
}

E_npcEthnicity U_populationSubsystem::RandEthnicity()
{
	// Fonte: distribuzioni demografiche reali (scelta artistica deliberata)
	const float r = RNG.FRand();
	if      (r < 0.58f) return E_npcEthnicity::White;
	else if (r < 0.74f) return E_npcEthnicity::Latino;
	else if (r < 0.86f) return E_npcEthnicity::Black;
	else if (r < 0.94f) return E_npcEthnicity::Asian;
	else if (r < 0.96f) return E_npcEthnicity::MiddleEastern;
	else                return E_npcEthnicity::Mixed;
}

E_npcPozStatus U_populationSubsystem::RandPozStatus(float HivRate, float UndetFrac)
{
	// Fix v2.3: af modula hiv_rate, None è il complemento
	const float pNone     = 1.f - HivRate;
	const float pPoz      = HivRate * (1.f - UndetFrac);
	const float r         = RNG.FRand();
	if      (r < pNone)          return E_npcPozStatus::None;
	else if (r < pNone + pPoz)   return E_npcPozStatus::Poz;
	else                         return E_npcPozStatus::PozUndet;
}

E_npcSafetyPractice U_populationSubsystem::RandSafetyPractice(E_npcPozStatus Poz)
{
	// Fix v2.3: PrEP solo HIV-negativi; HIV+ prendono ART
	const float r = RNG.FRand();
	switch (Poz)
	{
	case E_npcPozStatus::None:
		if      (r < 0.40f) return E_npcSafetyPractice::CondomOnly;
		else if (r < 0.70f) return E_npcSafetyPractice::PrEP;
		else                return E_npcSafetyPractice::Bareback;

	case E_npcPozStatus::PozUndet:
		return (r < 0.35f) ? E_npcSafetyPractice::ART_CondomOnly
		                   : E_npcSafetyPractice::ART_Bareback;

	case E_npcPozStatus::Poz:
		if      (r < 0.45f) return E_npcSafetyPractice::CondomOnly;
		else if (r < 0.85f) return E_npcSafetyPractice::Bareback;
		else                return E_npcSafetyPractice::ART_NonAdherent;

	default: return E_npcSafetyPractice::CondomOnly;
	}
}

// ================================================================
// TRIBE CLASSIFICATION
// Fonte: Moskowitz 2013, Grindr tribes 2013, Tran 2020
// ================================================================

FString U_populationSubsystem::ClassifyTribe(const F_npcProfile& P) const
{
	auto GetVar = [&](const TCHAR* V) -> float
	{
		if (FCString::Strcmp(V, TEXT("sl"))  == 0) return P.MySlim;
		if (FCString::Strcmp(V, TEXT("h"))   == 0) return P.MyHair;
		if (FCString::Strcmp(V, TEXT("m"))   == 0) return P.MyMuscle;
		if (FCString::Strcmp(V, TEXT("ma"))  == 0) return P.MyMasculinity;
		if (FCString::Strcmp(V, TEXT("po"))  == 0) return P.MyPolish;
		if (FCString::Strcmp(V, TEXT("age")) == 0) return P.myAgeNorm;
		return 0.5f;
	};

	FString BestTribe = TEXT("Average");
	float   BestScore = TRIBE_MIN_SCORE;

	for (int32 Ti = 0; Ti < TRIBE_COUNT; Ti++)
	{
		float WSS = 0.f;
		for (int32 Pi = 0; Pi < TRIBE_PROTOTYPES[Ti].Count; Pi++)
		{
			const FTribeProto& Proto = TRIBE_PROTOTYPES[Ti].Protos[Pi];
			const float Val    = GetVar(Proto.Var);
			const float Target = (FCString::Strcmp(Proto.Var, TEXT("age")) == 0)
			                     ? (Proto.Target - 18.f) / 82.f
			                     : Proto.Target;
			const float Diff = Val - Target;
			WSS += Proto.Weight * Diff * Diff;
		}
		const float Score = FMath::Exp(-WSS);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTribe = FString(TRIBE_PROTOTYPES[Ti].Name);
		}
	}
	return BestTribe;
}

// ================================================================
// KINKTYPES ASSIGNMENT
// Fonte: Parsons et al. 2010 (N=1214 MSM), Dean 2009
// ================================================================

void U_populationSubsystem::AssignKinkTypes(F_npcProfile& P)
{
	F_kinkFlags& K   = P.myKinkTypes;
	const float KL     = P.MyKinkLevel;
	const float SE     = P.MySexExtremity;

	// Vanilla totale: KL < 0.03 (~18% della popolazione)
	if (KL < 0.03f)
	{
		K.bVanilla = true;
		return;
	}

	bool bAnyActive = false;

	for (int32 i = 0; i < KINK_COUNT; i++)
	{
		const TCHAR*     Name = KINK_TYPES_DEF[i].Name;
		const FKinkDef&  D    = KINK_TYPES_DEF[i].Def;

		if (KL < D.kl_min) continue;

		float Prob = Clamp01(D.base + D.kl_w * KL + D.se_w * SE);

		// ExhibVoyeur: boost extra da SE (eroticizzazione ≠ body display)
		if (FCString::Strcmp(Name, TEXT("ExhibVoyeur")) == 0)
		{
			Prob = Clamp01(Prob + SE * 0.10f);
		}

		if (RNG.FRand() < Prob)
		{
			if (FCString::Strcmp(Name, TEXT("GroupSex"))    == 0) K.bGroupSex    = true;
			if (FCString::Strcmp(Name, TEXT("ExhibVoyeur")) == 0) K.bExhibVoyeur = true;
			if (FCString::Strcmp(Name, TEXT("LightBDSM"))   == 0) K.bLightBDSM   = true;
			if (FCString::Strcmp(Name, TEXT("Watersports"))  == 0) K.bWatersports = true;
			if (FCString::Strcmp(Name, TEXT("SM_Pain"))     == 0) K.bSM_Pain     = true;
			if (FCString::Strcmp(Name, TEXT("Fisting"))     == 0) K.bFisting     = true;
			if (FCString::Strcmp(Name, TEXT("PupPlay"))     == 0) K.bPupPlay     = true;
			if (FCString::Strcmp(Name, TEXT("Chemsex"))     == 0) K.bChemsex     = true;
			if (FCString::Strcmp(Name, TEXT("GearFetish"))  == 0) K.bGearFetish  = true;
			if (FCString::Strcmp(Name, TEXT("Roleplay"))    == 0) K.bRoleplay    = true;
			if (FCString::Strcmp(Name, TEXT("EdgePlay"))    == 0) K.bEdgePlay    = true;
			bAnyActive = true;
		}
	}

	// Breeding — condizionato a BT e SafetyPractice
	// Fonte: Dean 2009 — bottom riceve, boost da bareback
	{
		const float BtBoost   = 0.08f + (1.f - P.MyBottomTop) * 0.12f;
		const bool  bBareback = (P.MySafetyPractice == E_npcSafetyPractice::Bareback
		                      || P.MySafetyPractice == E_npcSafetyPractice::ART_Bareback);
		const float BbBoost   = bBareback ? 0.10f : 0.0f;
		const float BreedProb = Clamp01(0.02f + KL * 0.15f + SE * 0.12f + BtBoost + BbBoost);
		if (RNG.FRand() < BreedProb)
		{
			K.bBreeding = true;
			bAnyActive  = true;
		}
	}

	if (!bAnyActive) K.bVanilla = true;
}

// ================================================================
// CALCOLO ATTRAZIONE — Distanza Euclidea Pesata
// Fonte: Conroy-Beam 2016-2022 (N=14487, 45 paesi)
// ================================================================

float U_populationSubsystem::EthDesirability(E_npcEthnicity Eth) const
{
	// Fonte: OkCupid 2009 gay-specific
	switch (Eth)
	{
	case E_npcEthnicity::MiddleEastern: return 1.07f;
	case E_npcEthnicity::White:         return 1.00f;
	case E_npcEthnicity::Latino:        return 0.95f;
	case E_npcEthnicity::Mixed:         return 0.95f;
	case E_npcEthnicity::Asian:         return 0.85f;
	case E_npcEthnicity::Black:         return 0.80f;
	default:                           return 1.00f;
	}
}

float U_populationSubsystem::PozDesirability(E_npcPozStatus Poz) const
{
	// Fonte: Survey 2019, GLAAD 2022
	switch (Poz)
	{
	case E_npcPozStatus::None:     return 1.00f;
	case E_npcPozStatus::PozUndet: return 0.70f;
	case E_npcPozStatus::Poz:      return 0.36f;
	default:                      return 1.00f;
	}
}

float U_populationSubsystem::CalcVisualAttraction(int32 ObsId, int32 TgtId) const
{
	if (!Profiles.IsValidIndex(ObsId) || !Profiles.IsValidIndex(TgtId)) return 0.f;
	const F_npcProfile& A = Profiles[ObsId];
	const F_npcProfile& B = Profiles[TgtId];

	float SumWD2 = 0.f;
	SumWD2 += A.wVis_Muscle      * FMath::Square(A.myDesired_Muscle      - B.MyMuscle);
	SumWD2 += A.wVis_Beauty      * FMath::Square(A.myDesired_Beauty      - B.MyBeauty);
	SumWD2 += A.wVis_Slim        * FMath::Square(A.myDesired_Slim        - B.MySlim);
	SumWD2 += A.wVis_Masculinity * FMath::Square(A.myDesired_Masculinity - B.MyMasculinity);
	SumWD2 += A.wVis_Age         * FMath::Square(A.myDesired_Age         - B.myAgeNorm);
	SumWD2 += A.wVis_Hair        * FMath::Square(A.myDesired_Hair        - B.MyHair);
	SumWD2 += A.wVis_Height      * FMath::Square(A.myDesired_Height      - B.myHeightNorm);

	constexpr float Sigma = 0.13f;
	float Attr = FMath::Exp(-SumWD2 / (2.f * Sigma * Sigma));

	// Moltiplicatori post-distanza
	const float EthDev = EthDesirability(B.MyEthnicity) - 1.f;
	Attr *= FMath::Max(0.50f, 1.f + EthDev * A.myEthBias);

	const float PozDev = PozDesirability(B.MyPozStatus) - 1.f;
	Attr *= FMath::Max(0.20f, 1.f + PozDev * A.myPozBias);

	// Varianza relazionale (Eastwick 2024 — chimica imprevedibile)
	Attr += RNG.FRandRange(-0.03f, 0.03f);

	return Clamp01(Attr);
}

float U_populationSubsystem::CalcSexualAttraction(int32 ObsId, int32 TgtId) const
{
	if (!Profiles.IsValidIndex(ObsId) || !Profiles.IsValidIndex(TgtId)) return 0.f;
	const F_npcProfile& A = Profiles[ObsId];
	const F_npcProfile& B = Profiles[TgtId];

	float SumWD2 = 0.f;
	SumWD2 += A.wSex_BottomTop * FMath::Square(A.myDesired_BottomTop - B.MyBottomTop);
	SumWD2 += A.wSex_SubDom    * FMath::Square(A.myDesired_SubDom    - B.MySubDom);
	SumWD2 += A.wSex_KinkLevel * FMath::Square(A.myDesired_KinkLevel - B.MyKinkLevel);
	SumWD2 += A.wSex_PenisSize * FMath::Square(A.myDesired_PenisSize - B.MyPenisSize);

	constexpr float Sigma = 0.13f;
	float Attr = FMath::Exp(-SumWD2 / (2.f * Sigma * Sigma));

	// Dealbreaker BottomTop — Li et al. 2002 (necessità)
	const float BtDist = FMath::Abs(A.myDesired_BottomTop - B.MyBottomTop);
	if (BtDist > 0.60f) Attr *= 0.10f;

	Attr += RNG.FRandRange(-0.03f, 0.03f);
	return Clamp01(Attr);
}

float U_populationSubsystem::CalcAppAttraction(int32 ObsId, int32 TgtId) const
{
	if (!Profiles.IsValidIndex(ObsId) || !Profiles.IsValidIndex(TgtId)) return 0.f;
	const F_npcProfile& A = Profiles[ObsId];
	const F_npcProfile& B = Profiles[TgtId];

	float SumWD2 = 0.f;
	SumWD2 += A.wSin_Muscle      * FMath::Square(A.myDesired_Muscle      - B.MyMuscle);
	SumWD2 += A.wSin_Beauty      * FMath::Square(A.myDesired_Beauty      - B.MyBeauty);
	SumWD2 += A.wSin_Slim        * FMath::Square(A.myDesired_Slim        - B.MySlim);
	SumWD2 += A.wSin_Masculinity * FMath::Square(A.myDesired_Masculinity - B.MyMasculinity);
	SumWD2 += A.wSin_Age         * FMath::Square(A.myDesired_Age         - B.myAgeNorm);
	SumWD2 += A.wSin_Hair        * FMath::Square(A.myDesired_Hair        - B.MyHair);
	SumWD2 += A.wSin_Height      * FMath::Square(A.myDesired_Height      - B.myHeightNorm);
	SumWD2 += A.wSin_BottomTop   * FMath::Square(A.myDesired_BottomTop   - B.MyBottomTop);
	SumWD2 += A.wSin_SubDom      * FMath::Square(A.myDesired_SubDom      - B.MySubDom);
	SumWD2 += A.wSin_KinkLevel   * FMath::Square(A.myDesired_KinkLevel   - B.MyKinkLevel);
	SumWD2 += A.wSin_PenisSize   * FMath::Square(A.myDesired_PenisSize   - B.MyPenisSize);

	// σ=0.15 per 11 variabili (range distanza più ampio vs 7 o 4)
	constexpr float Sigma = 0.15f;
	float Attr = FMath::Exp(-SumWD2 / (2.f * Sigma * Sigma));

	const float EthDev = EthDesirability(B.MyEthnicity) - 1.f;
	Attr *= FMath::Max(0.50f, 1.f + EthDev * A.myEthBias);

	const float PozDev = PozDesirability(B.MyPozStatus) - 1.f;
	Attr *= FMath::Max(0.20f, 1.f + PozDev * A.myPozBias);

	// Dealbreaker BT: ×0.15 (meno severo di cruising — info preventiva in app)
	const float BtDist = FMath::Abs(A.myDesired_BottomTop - B.MyBottomTop);
	if (BtDist > 0.60f) Attr *= 0.15f;

	Attr += RNG.FRandRange(-0.03f, 0.03f);
	return Clamp01(Attr);
}

// ================================================================
// GetProfile
// ================================================================

const F_npcProfile& U_populationSubsystem::GetProfile(int32 NpcId) const
{
	check(Profiles.IsValidIndex(NpcId));
	return Profiles[NpcId];
}

// ================================================================
// SyncToDataTable — popola InspectionTable per debug editor
// ================================================================

void U_populationSubsystem::SetInspectionTable(UDataTable* Table)
{
    InspectionTable = Table;
    SyncToDataTable();
}



void U_populationSubsystem::SyncToDataTable()
{
	if (!InspectionTable) return;
	InspectionTable->EmptyTable();
	for (const F_npcProfile& P : Profiles)
	{
		const FName RowName(*FString::FromInt(P.NpcId));
		InspectionTable->AddRow(RowName, P);
	}
}

// ================================================================
// BuildAttractionMatrices
// Calcola tutte le attrazioni vis/sex una volta sola post-generazione.
// Runtime usa GetVisScore/GetSexScore — accesso O(1), nessun ricalcolo.
// N=700: ~3.8MB totali.
// ================================================================

void U_populationSubsystem::BuildAttractionMatrices()
{
	const int32 N = Profiles.Num();
	VisAttractionMatrix.SetNumZeroed(N * N);
	SexAttractionMatrix.SetNumZeroed(N * N);

	// §1.2 — AppAttractionMatrix: CalcAppAttraction() single-pass, tutti gli 11 variabili
	// con pesi normalizzati da letteratura (BottomTop dominante a 0.203).
	// Sostituisce la formula arbitraria 0.60×vis + 0.40×sex.
	AppAttractionMatrix.SetNumZeroed(N * N);

	for (int32 i = 0; i < N; i++)
	{
		for (int32 j = 0; j < N; j++)
		{
			if (i == j) continue;
			VisAttractionMatrix[i * N + j] = CalcVisualAttraction(i, j);
			SexAttractionMatrix[i * N + j] = CalcSexualAttraction(i, j);
			AppAttractionMatrix[i * N + j] = CalcAppAttraction(i, j);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("BuildAttractionMatrices: %d×%d = %d valori (vis+sex+app)"), N, N, N * N);
}

// ================================================================
// BuildMeshScaleCache
// Normalizza myPop_visAttraction su min/max popolazione → scala Z [0.5, 4.0].
// myPop_visAttraction = media attrazioni visive ricevute da tutti (Conroy-Beam 2018).
// ================================================================

void U_populationSubsystem::BuildMeshScaleCache()
{
	const int32 N = Profiles.Num();
	MeshScaleZCache.SetNumZeroed(N);

	// §1.3 — PopVisAttractionCache: myPop_visAttraction per ogni NPC
	// Media delle attrazioni visive ricevute da tutta la popolazione (colonna j della matrice).
	// Nota: già calcolato qui come PopVis[], ora esposto come PopVisAttractionCache.
	PopVisAttractionCache.SetNumZeroed(N);
	for (int32 j = 0; j < N; j++)
	{
		float Total = 0.f;
		for (int32 i = 0; i < N; i++)
		{
			if (i != j) Total += VisAttractionMatrix[i * N + j];
		}
		PopVisAttractionCache[j] = Total / FMath::Max(1, N - 1);
	}

	// §1.3 — PopSexAttractionCache: media attrazioni sessuali ricevute
	PopSexAttractionCache.SetNumZeroed(N);
	for (int32 j = 0; j < N; j++)
	{
		float Total = 0.f;
		for (int32 i = 0; i < N; i++)
		{
			if (i != j) Total += SexAttractionMatrix[i * N + j];
		}
		PopSexAttractionCache[j] = Total / FMath::Max(1, N - 1);
	}

	// §1.3 — PopAppAttractionCache: media attrazioni app-mode ricevute
	PopAppAttractionCache.SetNumZeroed(N);
	for (int32 j = 0; j < N; j++)
	{
		float Total = 0.f;
		for (int32 i = 0; i < N; i++)
		{
			if (i != j) Total += AppAttractionMatrix[i * N + j];
		}
		PopAppAttractionCache[j] = Total / FMath::Max(1, N - 1);
	}

	// §1.3 — OutVisAttractionCount / InVisAttractionCount
	// Usa VIS_STANDARD_THRESHOLD (0.35 fisso), NON myVisCurrThreshold adattiva.
	OutVisAttractionCount.SetNumZeroed(N);
	InVisAttractionCount.SetNumZeroed(N);
	for (int32 i = 0; i < N; i++)
	{
		for (int32 j = 0; j < N; j++)
		{
			if (i == j) continue;
			if (VisAttractionMatrix[i * N + j] >= VIS_STANDARD_THRESHOLD)
			{
				OutVisAttractionCount[i]++;
				InVisAttractionCount[j]++;
			}
		}
	}

	// §1.3 — VisAttractionBalance: myIn / myOut per NPC
	// >1 = posizione favorevole, <1 = frustrazione strutturale
	VisAttractionBalance.SetNumZeroed(N);
	for (int32 j = 0; j < N; j++)
	{
		const float Out = FMath::Max(1.f, static_cast<float>(OutVisAttractionCount[j]));
		VisAttractionBalance[j] = static_cast<float>(InVisAttractionCount[j]) / Out;
	}

	// MeshScaleZ: [0.5, 4.0] normalizzata su PopVisAttractionCache
	float MinVis = PopVisAttractionCache[0], MaxVis = PopVisAttractionCache[0];
	for (float V : PopVisAttractionCache)
	{
		MinVis = FMath::Min(MinVis, V);
		MaxVis = FMath::Max(MaxVis, V);
	}
	const float Range = FMath::Max(MaxVis - MinVis, 1e-4f);
	for (int32 j = 0; j < N; j++)
	{
		const float Normalized = (PopVisAttractionCache[j] - MinVis) / Range;
		MeshScaleZCache[j] = FMath::Lerp(0.5f, 4.0f, Normalized);
	}
}

// ================================================================
// Getter runtime — O(1)
// ================================================================

float U_populationSubsystem::GetVisScore(int32 ObsId, int32 TgtId) const
{
	if (ObsId == TgtId || !Profiles.IsValidIndex(ObsId) || !Profiles.IsValidIndex(TgtId))
		return 0.f;
	return VisAttractionMatrix[ObsId * Profiles.Num() + TgtId];
}

float U_populationSubsystem::GetSexScore(int32 ObsId, int32 TgtId) const
{
	if (ObsId == TgtId || !Profiles.IsValidIndex(ObsId) || !Profiles.IsValidIndex(TgtId))
		return 0.f;
	return SexAttractionMatrix[ObsId * Profiles.Num() + TgtId];
}

float U_populationSubsystem::GetContextualDesirability(int32 NpcId, const TArray<int32>& VisibleIds) const
{
	// Media attrazioni ricevute dai soli NPC attualmente visibili.
	// Chiamare solo quando NearbyNPCs cambia — non ogni tick.
	if (VisibleIds.Num() == 0) return 0.f;
	float Total = 0.f;
	for (int32 VId : VisibleIds)
		Total += GetVisScore(VId, NpcId);
	return Total / VisibleIds.Num();
}

float U_populationSubsystem::GetMeshScaleZ(int32 NpcId) const
{
	if (!MeshScaleZCache.IsValidIndex(NpcId)) return 1.f;
	return MeshScaleZCache[NpcId];
}

// ================================================================
// §1.2/1.3 — Getters cached vectors
// ================================================================

float U_populationSubsystem::GetAppScore(int32 ObsId, int32 TgtId) const
{
	if (ObsId == TgtId || !Profiles.IsValidIndex(ObsId) || !Profiles.IsValidIndex(TgtId))
		return 0.f;
	return AppAttractionMatrix[ObsId * Profiles.Num() + TgtId];
}

float U_populationSubsystem::GetPopVisAttraction(int32 NpcId) const
{
	if (!PopVisAttractionCache.IsValidIndex(NpcId)) return 0.f;
	return PopVisAttractionCache[NpcId];
}

float U_populationSubsystem::GetPopSexAttraction(int32 NpcId) const
{
	if (!PopSexAttractionCache.IsValidIndex(NpcId)) return 0.f;
	return PopSexAttractionCache[NpcId];
}

float U_populationSubsystem::GetPopAppAttraction(int32 NpcId) const
{
	if (!PopAppAttractionCache.IsValidIndex(NpcId)) return 0.f;
	return PopAppAttractionCache[NpcId];
}

int32 U_populationSubsystem::GetOutVisAttractionCount(int32 NpcId) const
{
	if (!OutVisAttractionCount.IsValidIndex(NpcId)) return 0;
	return OutVisAttractionCount[NpcId];
}

int32 U_populationSubsystem::GetInVisAttractionCount(int32 NpcId) const
{
	if (!InVisAttractionCount.IsValidIndex(NpcId)) return 0;
	return InVisAttractionCount[NpcId];
}

float U_populationSubsystem::GetVisAttractionBalance(int32 NpcId) const
{
	if (!VisAttractionBalance.IsValidIndex(NpcId)) return 0.f;
	return VisAttractionBalance[NpcId];
}

// ================================================================
// §1.4 — BuildPopulationMetrics
// Tutte le metriche scalari di popolazione. Chiamata una volta dopo
// BuildMeshScaleCache(). Opera su dati già calcolati — nessun ricalcolo
// delle attrazioni. O(N) per metriche per-variabile, O(N²) per pairs.
// ================================================================

void U_populationSubsystem::BuildPopulationMetrics()
{
	const int32 N = Profiles.Num();
	if (N == 0) return;

	const float Nf = static_cast<float>(N);

	// ── Gini coefficient su PopVisAttractionCache ─────────────────
	// Gini = (2 * sum(rank_i * val_i) / (N * sum(val_i))) - (N+1)/N
	{
		TArray<float> Sorted = PopVisAttractionCache;
		Sorted.Sort();
		float SumVal = 0.f, SumRankVal = 0.f;
		for (int32 i = 0; i < N; i++)
		{
			SumVal     += Sorted[i];
			SumRankVal += static_cast<float>(i + 1) * Sorted[i];
		}
		if (SumVal > 1e-6f)
			PopVisAttractionPolarization = (2.f * SumRankVal) / (Nf * SumVal) - (Nf + 1.f) / Nf;
		else
			PopVisAttractionPolarization = 0.f;
	}

	// ── Demand/supply ratio e desire gap per variabile ────────────
	// sum(myDesired_X) / sum(MyX) — scansione O(N) sui profili
	{
		double SumMyMuscle=0, SumDesiredMuscle=0;
		double SumMySlim=0,   SumDesiredSlim=0;
		double SumMyBeauty=0, SumDesiredBeauty=0;
		double SumMyMasc=0,   SumDesiredMasc=0;
		double SumMyHair=0,   SumDesiredHair=0;
		double SumMyAge=0,    SumDesiredAge=0;
		double SumMyBT=0,     SumDesiredBT=0;

		for (const F_npcProfile& P : Profiles)
		{
			SumMyMuscle      += P.MyMuscle;       SumDesiredMuscle      += P.myDesired_Muscle;
			SumMySlim        += P.MySlim;         SumDesiredSlim        += P.myDesired_Slim;
			SumMyBeauty      += P.MyBeauty;       SumDesiredBeauty      += P.myDesired_Beauty;
			SumMyMasc        += P.MyMasculinity;  SumDesiredMasc        += P.myDesired_Masculinity;
			SumMyHair        += P.MyHair;         SumDesiredHair        += P.myDesired_Hair;
			SumMyAge         += P.myAgeNorm;      SumDesiredAge         += P.myDesired_Age;
			SumMyBT          += P.MyBottomTop;    SumDesiredBT          += P.myDesired_BottomTop;
		}

		auto SafeRatio = [](double Num, double Den) -> float {
			return Den > 1e-6 ? static_cast<float>(Num / Den) : 1.f;
		};

		PopDemandSupplyRatio_Muscle      = SafeRatio(SumDesiredMuscle, SumMyMuscle);
		PopDemandSupplyRatio_Slim        = SafeRatio(SumDesiredSlim,   SumMySlim);
		PopDemandSupplyRatio_Beauty      = SafeRatio(SumDesiredBeauty, SumMyBeauty);
		PopDemandSupplyRatio_Masculinity = SafeRatio(SumDesiredMasc,   SumMyMasc);
		PopDemandSupplyRatio_Hair        = SafeRatio(SumDesiredHair,   SumMyHair);
		PopDemandSupplyRatio_Age         = SafeRatio(SumDesiredAge,    SumMyAge);
		PopDemandSupplyRatio_BottomTop   = SafeRatio(SumDesiredBT,     SumMyBT);

		PopDesireGap_Muscle      = static_cast<float>((SumDesiredMuscle - SumMyMuscle)   / N);
		PopDesireGap_Slim        = static_cast<float>((SumDesiredSlim   - SumMySlim)     / N);
		PopDesireGap_Beauty      = static_cast<float>((SumDesiredBeauty - SumMyBeauty)   / N);
		PopDesireGap_Masculinity = static_cast<float>((SumDesiredMasc   - SumMyMasc)     / N);
		PopDesireGap_Hair        = static_cast<float>((SumDesiredHair   - SumMyHair)     / N);
		PopDesireGap_Age         = static_cast<float>((SumDesiredAge    - SumMyAge)      / N);
		PopDesireGap_BottomTop   = static_cast<float>((SumDesiredBT     - SumMyBT)       / N);
	}

	// ── Pairs compatibility (O(N²) su matrici pre-calcolate) ─────
	// Kink incompatibility threshold — default 0.80, slider-configurabile in futuro
	constexpr float KINK_INCOMPAT_THRESHOLD = 0.80f;

	int64 TotalPairs       = 0;
	int64 VisMatchPairs    = 0;
	int64 VisSexMatchPairs = 0;
	int64 AppMatchPairs    = 0;
	int64 BtIncompatPairs  = 0;  // vis-matched che falliscono per BT
	int64 KinkIncompatPairs= 0;  // vis-matched che falliscono per kink

	for (int32 i = 0; i < N - 1; i++)
	{
		const F_npcProfile& A = Profiles[i];
		for (int32 j = i + 1; j < N; j++)
		{
			const F_npcProfile& B = Profiles[j];
			TotalPairs++;

			const float VisAB = VisAttractionMatrix[i * N + j];
			const float VisBA = VisAttractionMatrix[j * N + i];
			const bool  bVisMutual = (VisAB >= VIS_STANDARD_THRESHOLD && VisBA >= VIS_STANDARD_THRESHOLD);

			if (bVisMutual)
			{
				VisMatchPairs++;

				const float SexAB = SexAttractionMatrix[i * N + j];
				const float SexBA = SexAttractionMatrix[j * N + i];
				if (SexAB >= SEX_STANDARD_THRESHOLD && SexBA >= SEX_STANDARD_THRESHOLD)
					VisSexMatchPairs++;

				// BT incompatibility: dealbreaker attivo in almeno una direzione
				// Stessa soglia usata in CalcSexualAttraction (Li et al. 2002)
				const float BtDistAB = FMath::Abs(A.myDesired_BottomTop - B.MyBottomTop);
				const float BtDistBA = FMath::Abs(B.myDesired_BottomTop - A.MyBottomTop);
				if (BtDistAB > 0.60f || BtDistBA > 0.60f)
					BtIncompatPairs++;

				// Kink incompatibility: entrambe le direzioni superano la soglia
				const float KlDistAB = FMath::Abs(A.myDesired_KinkLevel - B.MyKinkLevel);
				const float KlDistBA = FMath::Abs(B.myDesired_KinkLevel - A.MyKinkLevel);
				if (KlDistAB > KINK_INCOMPAT_THRESHOLD && KlDistBA > KINK_INCOMPAT_THRESHOLD)
					KinkIncompatPairs++;
			}

			// App-mode match (indipendente da vis/sex)
			const float AppAB = AppAttractionMatrix[i * N + j];
			const float AppBA = AppAttractionMatrix[j * N + i];
			if (AppAB >= VIS_STANDARD_THRESHOLD && AppBA >= VIS_STANDARD_THRESHOLD)
				AppMatchPairs++;
		}
	}

	const float TotalF    = FMath::Max(1.f, static_cast<float>(TotalPairs));
	const float VisMatchF = FMath::Max(1.f, static_cast<float>(VisMatchPairs));

	PairsVisCompatibilityRate          = static_cast<float>(VisMatchPairs)    / TotalF;
	PairsVisSexCompatibilityRate_cruise       = static_cast<float>(VisSexMatchPairs) / TotalF;
	PairsFullCompatibilityRateApp      = static_cast<float>(AppMatchPairs)    / TotalF;
	PairsVisMatchedBottomTopIncompatibilityShare_cruise = static_cast<float>(BtIncompatPairs)  / VisMatchF;
	PairsKinkIncompatibilityShare      = static_cast<float>(KinkIncompatPairs)/ VisMatchF;

	UE_LOG(LogTemp, Log,
		TEXT("BuildPopulationMetrics: Gini=%.3f VisMatch=%.1f%% VisSexMatch=%.1f%% BtIncompat=%.1f%%"),
		PopVisAttractionPolarization,
		PairsVisCompatibilityRate * 100.f,
		PairsVisSexCompatibilityRate_cruise * 100.f,
		PairsVisMatchedBottomTopIncompatibilityShare_cruise * 100.f);


}
// ================================================================
// CalculateSubsetStats - Calcola metriche per gli NPC spawnati
// ================================================================
void U_populationSubsystem::CalculateSubsetStats(const TArray<int32>& SpawnedIDs)
{
	const int32 SubsetN = SpawnedIDs.Num();
	if (SubsetN <= 1) return;

	int32 TotalPairs = 0;
	int32 VisMatchPairs = 0;
	int32 VisSexMatchPairs = 0;
	int32 AppMatchPairs = 0;
	int32 BtIncompatPairs = 0;
	int32 KinkIncompatPairs = 0;

	// Costante locale per la soglia Kink (se non hai una macro globale)
	const float KINK_INCOMPAT_THRESHOLD = 0.5f;

	// Loop solo sugli ID presenti nel livello (Subset)
	for (int32 i = 0; i < SubsetN; ++i)
	{
		for (int32 j = i + 1; j < SubsetN; ++j)
		{
			int32 IdA = SpawnedIDs[i];
			int32 IdB = SpawnedIDs[j];

			TotalPairs++;

			const F_npcProfile& A = GetProfile(IdA);
			const F_npcProfile& B = GetProfile(IdB);

			// Calcoli Vis e Sex
			const float VisAB = VisAttractionMatrix[IdA * Profiles.Num() + IdB];
			const float VisBA = VisAttractionMatrix[IdB * Profiles.Num() + IdA];
			const float SexAB = SexAttractionMatrix[IdA * Profiles.Num() + IdB];
			const float SexBA = SexAttractionMatrix[IdB * Profiles.Num() + IdA];

			bool bVisMatch = (VisAB >= VIS_STANDARD_THRESHOLD && VisBA >= VIS_STANDARD_THRESHOLD);
			bool bSexMatch = (SexAB >= SEX_STANDARD_THRESHOLD && SexBA >= SEX_STANDARD_THRESHOLD);

			if (bVisMatch)
			{
				VisMatchPairs++;
				if (bSexMatch) VisSexMatchPairs++;

				// Controlli di incompatibilità bottom/top
				if ((A.MyBottomTop > 0.6f && B.MyBottomTop > 0.6f) || 
				    (A.MyBottomTop < 0.4f && B.MyBottomTop < 0.4f))
				{
					BtIncompatPairs++;
				}

				// Kink Incompatibility
				const float KlDistAB = FMath::Abs(A.myDesired_KinkLevel - B.MyKinkLevel);
				const float KlDistBA = FMath::Abs(B.myDesired_KinkLevel - A.MyKinkLevel);
				
				if (KlDistAB > KINK_INCOMPAT_THRESHOLD && KlDistBA > KINK_INCOMPAT_THRESHOLD) 
					KinkIncompatPairs++;
			}

			// App-mode match
			const float AppAB = AppAttractionMatrix[IdA * Profiles.Num() + IdB];
			const float AppBA = AppAttractionMatrix[IdB * Profiles.Num() + IdA];
			if (AppAB >= VIS_STANDARD_THRESHOLD && AppBA >= VIS_STANDARD_THRESHOLD)
				AppMatchPairs++;
		}
	}

	const float TotalF    = FMath::Max(1.f, static_cast<float>(TotalPairs));
	const float VisMatchF = FMath::Max(1.f, static_cast<float>(VisMatchPairs));

	// Aggiorna le variabili membro del Subsystem in base al Subset
	PairsVisCompatibilityRate = static_cast<float>(VisMatchPairs) / TotalF;
	PairsVisSexCompatibilityRate_cruise = static_cast<float>(VisSexMatchPairs) / TotalF;
	PairsFullCompatibilityRateApp = static_cast<float>(AppMatchPairs) / TotalF;
	PairsVisMatchedBottomTopIncompatibilityShare_cruise = static_cast<float>(BtIncompatPairs) / VisMatchF;
	PairsKinkIncompatibilityShare = static_cast<float>(KinkIncompatPairs) / VisMatchF;
}
