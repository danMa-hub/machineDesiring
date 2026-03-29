#include "U_overlaySubsystem.h"
#include "U_populationSubsystem.h"
#include "A_spawnManager.h"
#include "A_npcController.h"
#include "U_npcBrainComponent.h"
#include "NpcMemoryTypes.h"
#include "UW_mainOverlay.h"
#include "UW_populationPanel.h"
#include "UW_runtimePanel.h"
#include "UW_npcInspector.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "EngineUtils.h"

void U_overlaySubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(myTimer_DataRefresh);
		World->GetTimerManager().ClearTimer(myTimer_RuntimeRefresh);
		World->GetTimerManager().ClearTimer(myTimer_RuntimeSample);
	}
	Super::Deinitialize();
}


void U_overlaySubsystem::SetupOverlay(APlayerController* PC, TSubclassOf<UW_mainOverlay> OverlayClass)
{
	// Se il Player Controller non è valido o non è stata passata alcuna classe, interrompi
	if (!PC || !OverlayClass) 
	{
		UE_LOG(LogTemp, Error, TEXT("U_overlaySubsystem: SetupOverlay fallito (PC o OverlayClass nulli)."));
		return;
	}

	// Assegniamo direttamente la classe passata dal Blueprint
	MainOverlayClass = OverlayClass;

	// Da qui in poi prosegui con il tuo codice originale per creare il widget...
	// (Esempio: MainOverlay = CreateWidget<UW_mainOverlay>(PC, MainOverlayClass); )
	
	// Inizializza riferimenti al subsystem popolazione e allo spawn manager
	if (UGameInstance* GI = GetGameInstance())
		myPopSubsystem = GI->GetSubsystem<U_populationSubsystem>();

	UWorld* World = PC->GetWorld();
	if (World && !mySpawnManager)
	{
		for (TActorIterator<A_spawnManager> It(World); It; ++It)
		{
			mySpawnManager = *It;
			break;
		}
	}

	if (!MainOverlay)
	{
		MainOverlay = CreateWidget<UW_mainOverlay>(PC, MainOverlayClass);
		if (MainOverlay)
		{
			MainOverlay->AddToViewport();
		}
	}

	// Fallback: crea RuntimePanel programmaticamente se non è stato
	// assegnato tramite BindWidgetOptional nel Blueprint del main overlay.
	if (MainOverlay && !MainOverlay->RuntimePanel)
	{
		UW_runtimePanel* Panel = CreateWidget<UW_runtimePanel>(PC, UW_runtimePanel::StaticClass());
		if (Panel)
		{
			MainOverlay->RuntimePanel = Panel;
			Panel->SetVisibility(ESlateVisibility::Collapsed);
			Panel->AddToViewport(5);  // Z-order sopra il main overlay
		}
	}

	// Avvia timer aggiornamento PopulationPanel (ogni 3s)
	if (World)
	{
		World->GetTimerManager().SetTimer(myTimer_DataRefresh, this,
			&U_overlaySubsystem::TickOverlayData, 3.f, true);

		// Timer aggiornamento RuntimePanel (ogni 1s)
		World->GetTimerManager().SetTimer(myTimer_RuntimeRefresh, this,
			&U_overlaySubsystem::RefreshRuntimePanel, 1.f, true);

		// Timer campionamento dati NPC (ogni 10s, prima esecuzione dopo 10s)
		World->GetTimerManager().SetTimer(myTimer_RuntimeSample, this,
			&U_overlaySubsystem::SampleRuntimeData, 10.f, true, 10.f);
	}
}

void U_overlaySubsystem::RefreshPopulationPanel()
{
	if (!MainOverlay || !MainOverlay->PopulationPanel || !myPopSubsystem) return;

	// Usa solo gli NPC effettivamente spawnati — NpcById è il subset attivo
	TArray<int32> SpawnedIds;
	if (mySpawnManager)
		mySpawnManager->NpcById.GetKeys(SpawnedIds);

	MainOverlay->PopulationPanel->RefreshData(myPopSubsystem, SpawnedIds);
}

void U_overlaySubsystem::TickOverlayData()
{
	// Aggiorna statistiche popolazione ogni 3s (include timing post-spawn)
	RefreshPopulationPanel();

	if (myInspectedNpcId >= 0)
		RefreshNpcInspector();
}

void U_overlaySubsystem::RefreshNpcInspector()
{
	if (!MainOverlay || !MainOverlay->NpcInspector || !myPopSubsystem || !mySpawnManager) return;

	U_npcBrainComponent** Found = mySpawnManager->NpcById.Find(myInspectedNpcId);
	if (!Found || !(*Found)) return;

	U_npcBrainComponent* Brain = *Found;

	MainOverlay->NpcInspector->RefreshNpc(
		myInspectedNpcId,
		myPopSubsystem->GetPopVisAttraction(myInspectedNpcId),
		myPopSubsystem->GetPopSexAttraction(myInspectedNpcId),
		myPopSubsystem->GetPopAppAttraction(myInspectedNpcId),
		myPopSubsystem->GetVisAttractionBalance(myInspectedNpcId),
		Brain->myVisStartThreshold,
		Brain->myVisCurrThreshold,
		Brain->myVisMinThreshold,
		Brain->mySexStartThreshold,
		Brain->mySexCurrThreshold,
		Brain->mySexMinThreshold,
		myPopSubsystem->GetOutVisAttractionCount(myInspectedNpcId),
		myPopSubsystem->GetInVisAttractionCount(myInspectedNpcId),
		Brain->myDesiredRank.Num(),
		Brain->myCurrentState,
		[&]() -> E_cruisingSubState {
			APawn* Pawn = Cast<APawn>(Brain->GetOwner());
			if (Pawn)
				if (A_npcController* Ctrl = Cast<A_npcController>(Pawn->GetController()))
					return Ctrl->GetCruisingSubState();
			return E_cruisingSubState::RandomWalk;
		}()
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

void U_overlaySubsystem::OnMatingStarted(int32 IdA, int32 IdB, float VisThreshA, float VisThreshB)
{
	float SimTime = 0.f;
	if (UWorld* World = GetWorld()) SimTime = World->GetTimeSeconds();

	// Registra evento
	F_matchEvent Evt;
	Evt.IdA = IdA;  Evt.IdB = IdB;
	Evt.VisThreshA = VisThreshA;  Evt.VisThreshB = VisThreshB;
	Evt.TimeSeconds = SimTime;
	myMatchLog.Add(Evt);

	// Formatta riga per il display (più recente in cima)
	const int32 Mins = FMath::FloorToInt(SimTime / 60.f);
	const int32 Secs = FMath::FloorToInt(SimTime) % 60;
	FString Line = FString::Printf(
		TEXT("  %02d:%02d    #%-4d ↔ #%-4d    vis_A=%.3f    vis_B=%.3f"),
		Mins, Secs, IdA, IdB, VisThreshA, VisThreshB);
	myMatchLines.Insert(Line, 0);

	// Tiene solo le ultime 30 righe
	if (myMatchLines.Num() > 30)
		myMatchLines.SetNum(30);
}

void U_overlaySubsystem::RefreshRuntimePanel()
{
	if (!MainOverlay || !MainOverlay->RuntimePanel) return;

	float SimTime = 0.f;
	if (UWorld* World = GetWorld()) SimTime = World->GetTimeSeconds();

	MainOverlay->RuntimePanel->RefreshRuntimeData(
		SimTime,
		myMatchLog.Num(),
		myMatchLines,
		myH_VisCurrThresh,
		myH_DesiredListLen,
		myAvgGaveUp);
}

void U_overlaySubsystem::SampleRuntimeData()
{
	if (!mySpawnManager) return;

	static constexpr int32 BINS = 20;
	myH_VisCurrThresh.Init(0, BINS);
	myH_DesiredListLen.Init(0, 21);   // bin[i] = NPC con lista di lunghezza i

	int32 TotalGaveUp = 0;
	int32 NpcCount    = 0;

	for (auto& Pair : mySpawnManager->NpcById)
	{
		U_npcBrainComponent* Brain = Pair.Value;
		if (!Brain) continue;
		NpcCount++;

		// Distribuzione vis_curr_threshold (range atteso [0.05, 0.35])
		const int32 VisIdx = FMath::Clamp(
			FMath::FloorToInt(Brain->myVisCurrThreshold * BINS), 0, BINS - 1);
		myH_VisCurrThresh[VisIdx]++;

		// Distribuzione lunghezza lista desiderati (0..20)
		const int32 ListLen = FMath::Clamp(Brain->myDesiredRank.Num(), 0, 20);
		myH_DesiredListLen[ListLen]++;

		// Conteggio GaveUp in mySocialMemory
		for (auto& MemPair : Brain->mySocialMemory)
			if (MemPair.Value.state == E_otherIDState::GaveUp)
				TotalGaveUp++;
	}

	myAvgGaveUp = (NpcCount > 0) ? (float)TotalGaveUp / NpcCount : 0.f;
}
