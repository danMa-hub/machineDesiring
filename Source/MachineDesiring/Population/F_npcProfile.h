#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "F_npcProfile.generated.h"

// ================================================================
// ENUMs
// ================================================================

UENUM(BlueprintType)
enum class E_npcEthnicity : uint8
{
    White          UMETA(DisplayName="White"),
    Latino         UMETA(DisplayName="Latino"),
    Black          UMETA(DisplayName="Black"),
    Asian          UMETA(DisplayName="Asian"),
    MiddleEastern  UMETA(DisplayName="MiddleEastern"),
    Mixed          UMETA(DisplayName="Mixed"),
};

UENUM(BlueprintType)
enum class E_npcPozStatus : uint8
{
    None       UMETA(DisplayName="None"),
    PozUndet   UMETA(DisplayName="PozUndet"),
    Poz        UMETA(DisplayName="Poz"),
};

UENUM(BlueprintType)
enum class E_npcSafetyPractice : uint8
{
    CondomOnly       UMETA(DisplayName="CondomOnly"),
    PrEP             UMETA(DisplayName="PrEP"),
    Bareback         UMETA(DisplayName="Bareback"),
    ART_CondomOnly   UMETA(DisplayName="ART_CondomOnly"),
    ART_Bareback     UMETA(DisplayName="ART_Bareback"),
    ART_NonAdherent  UMETA(DisplayName="ART_NonAdherent"),
};

// ================================================================
// F_kinkFlags — bitmask per DataTable (no TArray nei DataTable)
// Fonte: Parsons et al. 2010 (N=1214 MSM)
// ================================================================
USTRUCT(BlueprintType)
struct F_kinkFlags
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bVanilla       = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bGroupSex      = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bExhibVoyeur   = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bLightBDSM     = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bWatersports   = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bSM_Pain       = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bFisting       = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPupPlay       = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bChemsex       = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bGearFetish    = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRoleplay      = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bEdgePlay      = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bBreeding      = false;
};

// ================================================================
// F_npcProfile — IndividualProfile + MatingPreferences + Pesi
// FTableRowBase → compatibile DataTable per ispezione editor
// ================================================================
USTRUCT(BlueprintType)
struct F_npcProfile : public FTableRowBase
{
    GENERATED_BODY()

    // ── Identità ────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    int32 NpcId = -1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    E_npcEthnicity MyEthnicity = E_npcEthnicity::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    E_npcPozStatus MyPozStatus = E_npcPozStatus::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    E_npcSafetyPractice MySafetyPractice = E_npcSafetyPractice::CondomOnly;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FString myTribeTag = "Average";   // SOLO LOD/debug

    // ── Corpo ────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float MyAge = 38.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float myAgeNorm = 0.f;           // (MyAge-18)/82

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float MyHeight = 177.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float myHeightNorm = 0.f;        // (MyHeight-160)/35

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float MyMuscle = 0.30f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float MySlim = 0.48f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float MyHair = 0.46f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float MyBodyMod = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Body")
    float MyBodyDisplay = 0.22f;     // UE5 vestiti LOD

    // ── Presenza ─────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presence")
    float MyBeauty = 0.40f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presence")
    float MyMasculinity = 0.62f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presence")
    float MyPolish = 0.44f;          // fuori dal match

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presence")
    float MyQueerness = 0.46f;       // fuori dal match

    // ── Sessuale ─────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sexual")
    float MyBottomTop = 0.50f;       // 0=bottom, 1=top

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sexual")
    float MySubDom = 0.44f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sexual")
    float MyKinkLevel = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sexual")
    float MySexExtremity = 0.32f;    // condiziona KinkTypes

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sexual")
    float MySexActDepth = 0.38f;     // fuori dal match

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sexual")
    float MyPenisSize = 0.50f;

    // ── Psicologiche ─────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Psych")
    float MyOpenClosed = 0.55f;      // futuro uso relazionale

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Psych")
    float MyRelDepth = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Psych")
    float MySelfEsteem = 0.48f;      // BehaviorTree

    // ── KinkTypes ────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Kink")
    F_kinkFlags myKinkTypes;

    // ================================================================
    // MatingPreferences — myDesired_*
    // ================================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_Muscle = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_Slim = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_Beauty = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_Masculinity = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_Height = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_Hair = 0.46f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_Age = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_BottomTop = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_SubDom = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_KinkLevel = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preferences")
    float myDesired_PenisSize = 0.50f;   // BT-condizionale, fuori ASSORT_COEFFS

    // ================================================================
    // Pesi individuali normalizzati — flat (TMap non supportata DataTable)
    // ================================================================

    // myWeightsVisual (7 variabili — cruising fase 1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsVisual")
    float wVis_Muscle = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsVisual")
    float wVis_Beauty = 0.23f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsVisual")
    float wVis_Slim = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsVisual")
    float wVis_Masculinity = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsVisual")
    float wVis_Age = 0.07f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsVisual")
    float wVis_Hair = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsVisual")
    float wVis_Height = 0.04f;

    // myWeightsSexual (4 variabili — cruising fase 2)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSexual")
    float wSex_BottomTop = 0.40f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSexual")
    float wSex_SubDom = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSexual")
    float wSex_KinkLevel = 0.17f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSexual")
    float wSex_PenisSize = 0.15f;

    // myWeightsSingle (11 variabili — app single-pass)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_BottomTop = 0.203f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_Muscle = 0.142f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_SubDom = 0.142f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_Beauty = 0.117f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_KinkLevel = 0.086f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_Slim = 0.076f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_Masculinity = 0.076f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_PenisSize = 0.076f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_Age = 0.036f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_Hair = 0.025f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WeightsSingle")
    float wSin_Height = 0.020f;

    // ── Bias osservatore (fisso per persona) ────────────────────
    // Fonte: Callander 2015 (etnia), Smit 2012 (PozStatus)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bias")
    float myEthBias = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bias")
    float myPozBias = 1.0f;
};
