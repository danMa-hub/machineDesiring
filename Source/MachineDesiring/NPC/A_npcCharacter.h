#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NpcMemoryTypes.h"
#include "A_npcCharacter.generated.h"

// ================================================================
// A_npcCharacter
// Character base per ogni NPC della simulazione.
// Contiene: identità, FSM, memoria sociale, soglie, segnalazione.
// Logica cruising/mating → U_cruisingLogic / U_matingLogic
// Logica percezione      → A_npcController
// ================================================================

UCLASS()
class MACHINEDESIRING_API A_npcCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	A_npcCharacter();

	// ── Identità ─────────────────────────────────────────────────

	// ID progressivo — indice nella matrice di U_populationSubsystem
	UPROPERTY(BlueprintReadOnly, Category="NPC|Identity")
	int32 myId = -1;

	// Chiamato da A_spawnManager subito dopo lo spawn.
	// Assegna myId, recupera il profilo, inizializza soglie.
	UFUNCTION(BlueprintCallable, Category="NPC|Identity")
	void InitWithId(int32 Id);

	// ── FSM ──────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="NPC|State")
	E_myState myCurrentState = E_myState::Cruising;

	UFUNCTION(BlueprintCallable, Category="NPC|State")
	void SetState(E_myState NewState);

	// ── Memoria sociale ──────────────────────────────────────────

	// Database relazioni: tutti gli NPC mai visti
	// Accesso O(1) per ID — non UPROPERTY (int32 key, no GC needed)
	TMap<int32, F_otherIDMemory> mySocialMemory;

	// Array ordinato per priorità — max 20 elementi (stato Desired)
	// Riordinato on-event, mai ogni tick
	UPROPERTY(BlueprintReadOnly, Category="NPC|Memory")
	TArray<int32> myDesiredRank;

	// Pre-filtro hard: visAttraction < myVisMinThreshold
	// Popolato al primo avvistamento, mai rivalutato
	TSet<int32> myStaticVisBlacklist;

	// ── Soglie visive ────────────────────────────────────────────

	// Valore iniziale — snapshot immutabile per debug e reset
	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float myVisStartThreshold = 0.35f;

	// Soglia corrente — solo discendente nel tempo
	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float myVisCurrThreshold = 0.35f;

	// Hard limit — immutabile, assegnato in InitWithId
	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float myVisMinThreshold = 0.10f;

	// ── Soglie sessuali ──────────────────────────────────────────

	// Valore iniziale — snapshot immutabile per debug e reset
	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float mySexStartThreshold = 0.50f;

	// Soglia corrente — solo discendente nel tempo
	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float mySexCurrThreshold = 0.50f;

	// Hard limit — immutabile, default 0.40 (Algoritmo_Cruising §2.1)
	UPROPERTY(BlueprintReadOnly, Category="NPC|Threshold")
	float mySexMinThreshold = 0.40f;

	// ── Segnalazione ─────────────────────────────────────────────

	// TRUE durante l'intero ciclo handshake (3s: 1.5s attivo + 1.5s pausa)
	// Mentre TRUE: cieco a nuove interazioni, inattaccabile
	UPROPERTY(BlueprintReadOnly, Category="NPC|Signaling")
	bool b_myIsSignaling = false;

	// NPC con cui è in corso l'handshake atomico
	// TWeakObjectPtr: si annulla automaticamente se il target viene distrutto
	TWeakObjectPtr<A_npcCharacter> mySignalTarget;

	// Velocità base assegnata in InitWithId — varia per NPC [70, 100] cm/s
	UPROPERTY(BlueprintReadOnly, Category="NPC|Movement")
	float myMaxWalkSpeed = 80.f;

	// ── Materiali per sub-stato ───────────────────────────────────

	// Assegnabili nel Blueprint BP_NPC_Character
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Appearance")
	UMaterialInterface* Mat_RandomWalk = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Appearance")
	UMaterialInterface* Mat_Idle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Appearance")
	UMaterialInterface* Mat_Mating = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Appearance")
	UMaterialInterface* Mat_TargetWalk = nullptr;

	// Applica il materiale corrispondente al sub-stato su tutti gli slot del mesh
	void SetSubStateMaterial(E_cruisingSubState SubState);

	// Velocità di rotazione (deg/s) — usata da CharacterMovement e dal pre-rotation timer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Movement")
	float myRotationRate = 60.f;

protected:

	virtual void BeginPlay() override;

private:

	// Inizializza soglie min con varianza per-NPC
	// myVisMinThreshold: gauss(0.10, 0.02) — lieve varianza individuale
	// mySexMinThreshold: 0.40 fisso (hard limit algoritmico)
	void InitThresholds();
};
