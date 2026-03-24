// Fill out your copyright notice in the Description page of Project Settings.

#include "A_playerController.h"
#include "A_npcController.h"

void A_playerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (WasInputKeyJustPressed(EKeys::One))
		ToggleNpcDebugDraw();
	if (WasInputKeyJustPressed(EKeys::Two))
		ToggleNpcDebugId();
}

void A_playerController::ToggleNpcDebugDraw()
{
	A_npcController::bDebugDrawEnabled = !A_npcController::bDebugDrawEnabled;
	UE_LOG(LogTemp, Log, TEXT("NPC debug draw: %s"),
		A_npcController::bDebugDrawEnabled ? TEXT("ON") : TEXT("OFF"));
}

void A_playerController::ToggleNpcDebugId()
{
	A_npcController::bDebugIdEnabled = !A_npcController::bDebugIdEnabled;
	UE_LOG(LogTemp, Log, TEXT("NPC debug ID: %s"),
		A_npcController::bDebugIdEnabled ? TEXT("ON") : TEXT("OFF"));
}
