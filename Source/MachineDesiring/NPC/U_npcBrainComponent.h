#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NpcMemoryTypes.h"
#include "F_npcProfile.h"
#include "U_npcBrainComponent.generated.h"

// ================================================================
// U_npcBrainComponent
// Porta tutta l'identità e lo stato runtime di un NPC.
// Aggiungibile a qualsiasi APawn — GASP, ThirdPerson, MetaHuman.
// A_npcController accede ai dati tramite FindComponentByClass.
// ================================================================

UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class MACHINEDESIRING_API U_npcBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	U_npcBrainComponent();

	// ── Identità ───────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="NPC|Identity")
	int32 myId = -1;

	// Chiamato da A_spawnManager subito dopo lo spawn.
	// Assegna myId, inizializza soglie, estrae parametri Mutable.
	UFUNCTION(BlueprintCallable, Category="NPC|Identity")
	void InitWithId(int32 Id);

	// ── FSM ───────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="NPC|State")
	E_myState myCurrentState = E_myState::Cruising;

	UFUNCTION(BlueprintCallable, Category="NPC|State")
	void SetState(E_myState NewState);

	// ── Memoria sociale ───────────────────────────────────────

	// Database relazioni — no UPROPERTY, int32 key
	TMap<int32, F_otherIDMemory> mySocialMemory;

	// Array ordinato per VisScore desc — max 20 entry Desired
	UPROPERTY(BlueprintReadOnly, Category="NPC|Memory")
	TArray<int32> myDesiredRank;

	// Pre-filtro hard: VisScore < myVisMinThreshold → mai rivalutato
	TSet<int32> myStaticVisBlacklist;

	// ── Soglie visive ─────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float myVisStartThreshold = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float myVisCurrThreshold  = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float myVisMinThreshold   = 0.10f;

	// ── Soglie sessuali ───────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float mySexStartThreshold = 0.50f;

	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float mySexCurrThreshold  = 0.50f;

	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float mySexMinThreshold   = 0.40f;

	// ── Segnalazione ──────────────────────────────────────────

	// TRUE durante l'intero ciclo handshake — blocca nuove interazioni
	UPROPERTY(BlueprintReadOnly, Category="NPC|Signaling")
	bool b_myIsSignaling = false;

	// BrainComponent del partner nell'handshake corrente
	TWeakObjectPtr<U_npcBrainComponent> mySignalTarget;

	// ── Movimento ────────────────────────────────────────────

	// Velocità base [70,100] cm/s — deterministica per-NPC (seed = myId + 10000)
	UPROPERTY(BlueprintReadOnly, Category="NPC|Movement")
	float myMaxWalkSpeed = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Movement")
	float myRotationRate = 60.f;

	// ── Parametri Mutable ────────────────────────────────────
	// Popolati da InitWithId() → F_npcProfile.
	// Il Blueprint li legge in BeginPlay per configurare UCustomizableSkeletalComponent.

	UPROPERTY(BlueprintReadOnly, Category="NPC|Mutable")
	float MutableMuscle      = 0.f;   // MyMuscle [0,1]

	UPROPERTY(BlueprintReadOnly, Category="NPC|Mutable")
	float MutableSlim        = 0.f;   // MySlim [0,1]

	UPROPERTY(BlueprintReadOnly, Category="NPC|Mutable")
	float MutableHeight      = 0.f;   // myHeightNorm [0,1] — scala verticale

	UPROPERTY(BlueprintReadOnly, Category="NPC|Mutable")
	float MutableAge         = 0.f;   // myAgeNorm [0,1]

	UPROPERTY(BlueprintReadOnly, Category="NPC|Mutable")
	float MutableBodyDisplay = 0.f;   // MyBodyDisplay [0=vestito, 1=nudo]

	UPROPERTY(BlueprintReadOnly, Category="NPC|Mutable")
	float MutableHair        = 0.f;   // MyHair [0,1]

	UPROPERTY(BlueprintReadOnly, Category="NPC|Mutable")
	E_npcEthnicity MutableEthnicity = E_npcEthnicity::White;

	// Chiamato da A_spawnManager dopo InitWithId.
	// Implementa nel Blueprint: leggi i Mutable* e configura UCustomizableSkeletalComponent.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|Mutable")
	void BP_ApplyMutableProfile();

	// ── Blueprint events — stato ─────────────────────────────

	// Chiamato ogni volta che myCurrentState cambia.
	// Implementa nel Blueprint per aggiornare animazioni / effetti.
	UFUNCTION(BlueprintImplementableEvent, Category="NPC|State")
	void BP_OnStateChanged(E_myState NewState);

	// ── Materiali debug per sub-stato ────────────────────────
	// Assegnabili nel Blueprint. SetSubStateMaterial() li applica su tutti gli slot.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Debug")
	UMaterialInterface* Mat_RandomWalk = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Debug")
	UMaterialInterface* Mat_Idle       = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Debug")
	UMaterialInterface* Mat_Mating     = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Debug")
	UMaterialInterface* Mat_TargetWalk = nullptr;

	// Applica il materiale debug su tutti gli USkeletalMeshComponent dell'owner
	void SetSubStateMaterial(E_cruisingSubState SubState);

private:

	void InitThresholds();
	void ExtractMutableParams(const F_npcProfile& Profile);
};
