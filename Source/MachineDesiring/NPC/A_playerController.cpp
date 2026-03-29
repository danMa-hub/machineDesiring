// Fill out your copyright notice in the Description page of Project Settings.

#include "A_playerController.h"
#include "A_npcController.h"
#include "U_overlaySubsystem.h"
#include "Kismet/GameplayStatics.h"

U_overlaySubsystem* A_playerController::GetOverlaySub() const
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	return GI ? GI->GetSubsystem<U_overlaySubsystem>() : nullptr;
}

void A_playerController::BeginPlay()
{
	Super::BeginPlay();

	// Assicura che il gioco riceva input anche con widget in viewport
	SetInputMode(FInputModeGameOnly());

	// Reset stato debug (le statiche persistono tra sessioni PIE)
	A_npcController::bDebugDrawEnabled = false;
	A_npcController::bDebugIdEnabled   = false;
}

void A_playerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (WasInputKeyJustPressed(EKeys::One))
		ToggleNpcDebugDraw();
	if (WasInputKeyJustPressed(EKeys::Two))
		ToggleNpcDebugId();
	if (WasInputKeyJustPressed(EKeys::Three))
		TogglePopulationPanel();
	if (WasInputKeyJustPressed(EKeys::Four))
		ToggleRuntimePanel();
}

void A_playerController::TogglePopulationPanel()
{
	U_overlaySubsystem* Sub = GetOverlaySub();
	if (!Sub) return;

	const bool bVisible = Sub->MainOverlay &&
		Sub->MainOverlay->ActiveView == E_overlayView::PopulationPanel;

	if (bVisible)
	{
		Sub->ShowView(E_overlayView::None);
	}
	else
	{
		Sub->RefreshPopulationPanel(); // aggiorna dati prima di mostrare
		Sub->ShowView(E_overlayView::PopulationPanel);
	}
}

void A_playerController::ToggleRuntimePanel()
{
	U_overlaySubsystem* Sub = GetOverlaySub();
	if (!Sub) return;

	const bool bVisible = Sub->MainOverlay &&
		Sub->MainOverlay->ActiveView == E_overlayView::RuntimePanel;

	Sub->ShowView(bVisible ? E_overlayView::None : E_overlayView::RuntimePanel);
}

void A_playerController::ToggleNpcDebugDraw()
{
	A_npcController::bDebugDrawEnabled = !A_npcController::bDebugDrawEnabled;
}

void A_playerController::ToggleNpcDebugId()
{
	A_npcController::bDebugIdEnabled = !A_npcController::bDebugIdEnabled;
}
