#include "U_overlaySubsystem.h"
#include "U_populationSubsystem.h"
#include "A_spawnManager.h"
#include "A_npcCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void U_overlaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void U_overlaySubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(myTimer_DataRefresh);

	Super::Deinitialize();
}

void U_overlaySubsystem::SetupOverlay(APlayerController* PC)
{
	if (!PC || !MainOverlayClass) return;

	// Cache subsystem popolazione
	myPopSubsystem = GetGameInstance()->GetSubsystem<U_populationSubsystem>();

	// Cache SpawnManager
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(PC->GetWorld(), A_spawnManager::StaticClass(), Found);
	if (Found.Num() > 0)
		mySpawnManager = Cast<A_spawnManager>(Found[0]);

	// Crea widget e aggiunge al viewport
	MainOverlay = CreateWidget<UW_mainOverlay>(PC, MainOverlayClass);
	if (MainOverlay)
	{
		MainOverlay->AddToViewport(5);

		// Carica dati statici View 1 una sola volta
		RefreshPopulationPanel();

		// Timer 3s per View 2 (runtime)
		if (UWorld* World = PC->GetWorld())
		{
			World->GetTimerManager().SetTimer(
				myTimer_DataRefresh,
				FTimerDelegate::CreateUObject(this, &U_overlaySubsystem::TickOverlayData),
				3.f,
				true);
		}
	}
}

void U_overlaySubsystem::RefreshPopulationPanel()
{
	if (!MainOverlay || !MainOverlay->PopulationPanel || !myPopSubsystem) return;

	MainOverlay->PopulationPanel->RefreshData(
		myPopSubsystem->GetPairsVisCompatibilityRate(),
		myPopSubsystem->GetPairsVisSexCompatibilityRate_cruise(),
		myPopSubsystem->GetPairsFullCompatibilityRateApp(),
		myPopSubsystem->GetPopVisAttractionPolarization(),
		myPopSubsystem->GetPairsVisMatchedBottomTopIncompatibilityShare_cruise(),
		myPopSubsystem->GetPairsKinkIncompatibilityShare(),
		myPopSubsystem->GetDemandSupplyRatio_Muscle(),
		myPopSubsystem->GetDemandSupplyRatio_Beauty(),
		myPopSubsystem->GetDemandSupplyRatio_BottomTop(),
		myPopSubsystem->GetDesireGap_Muscle(),
		myPopSubsystem->GetDesireGap_Beauty(),
		myPopSubsystem->GetDesireGap_Age(),
		myPopSubsystem->GetDesireGap_BottomTop()
	);
}

void U_overlaySubsystem::TickOverlayData()
{
	// View 2 — aggiornamento runtime (placeholder: la View 2 sarà implementata in UW_populationPanel)
	// Per ora aggiorna solo l'NPC inspector se attivo
	if (myInspectedNpcId >= 0)
		RefreshNpcInspector();
}

void U_overlaySubsystem::RefreshNpcInspector()
{
	if (!MainOverlay || !MainOverlay->NpcInspector || !myPopSubsystem || !mySpawnManager) return;

	A_npcCharacter** Found = mySpawnManager->NpcById.Find(myInspectedNpcId);
	if (!Found || !(*Found)) return;

	A_npcCharacter* NPC = *Found;

	MainOverlay->NpcInspector->RefreshNpc(
		myInspectedNpcId,
		myPopSubsystem->GetPopVisAttraction(myInspectedNpcId),
		myPopSubsystem->GetPopSexAttraction(myInspectedNpcId),
		myPopSubsystem->GetPopAppAttraction(myInspectedNpcId),
		myPopSubsystem->GetVisAttractionBalance(myInspectedNpcId),
		NPC->myVisStartThreshold,
		NPC->myVisCurrThreshold,
		NPC->myVisMinThreshold,
		NPC->mySexStartThreshold,
		NPC->mySexCurrThreshold,
		NPC->mySexMinThreshold,
		myPopSubsystem->GetOutVisAttractionCount(myInspectedNpcId),
		myPopSubsystem->GetInVisAttractionCount(myInspectedNpcId),
		NPC->myDesiredRank.Num(),
		NPC->myCurrentState,
		E_cruisingSubState::RandomWalk  // TODO: esporre myCruisingSubState da A_npcController
	);
}

void U_overlaySubsystem::ShowView(E_overlayView View)
{
	if (MainOverlay) MainOverlay->ShowView(View);
}

void U_overlaySubsystem::InspectNpc(int32 NpcId)
{
	myInspectedNpcId = NpcId;
	RefreshNpcInspector();
	ShowView(E_overlayView::NpcInspector);
}

void U_overlaySubsystem::OnSignalExchanged(int32 IdA, int32 IdB, float ValA, float ValB)
{
	if (!MainOverlay || !myPopSubsystem) return;

	const float VisAB = myPopSubsystem->GetVisScore(IdA, IdB);
	const float VisBA = myPopSubsystem->GetVisScore(IdB, IdA);
	MainOverlay->ShowSignalEvent(IdA, IdB, ValA, ValB, VisAB, VisBA);
}
