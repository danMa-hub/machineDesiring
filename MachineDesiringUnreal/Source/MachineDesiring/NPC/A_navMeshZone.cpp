#include "A_navMeshZone.h"
#include "NavigationSystem.h"
#include "Components/BillboardComponent.h"

// ================================================================
// Inizializzazione variabili statiche
// ================================================================
TArray<A_navMeshZone*> A_navMeshZone::allIdleZones;
TArray<A_navMeshZone*> A_navMeshZone::allMatingZones;

// ================================================================
// Costruttore — crea componenti, configura root generica
// ================================================================
A_navMeshZone::A_navMeshZone()
{
    PrimaryActorTick.bCanEverTick = false;  // nessun tick necessario

    // 1. ROOT GENERICA — La cosa migliore per evitare deformazioni a cascata
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // 1.5 ICONA EDITOR — Rende l'Actor facilissimo da cliccare nel livello
    editorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
    editorIcon->SetupAttachment(RootComponent);
    editorIcon->bHiddenInGame = true;

    // 2. SFERA — ora è "figlia" della root generica
    sphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    sphereComp->SetupAttachment(RootComponent);
    sphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    sphereComp->SetSphereRadius(sphereRadius);

    // 3. BOX — anch'esso "figlio" della root generica
    boxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
    boxComp->SetupAttachment(RootComponent);
    boxComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    boxComp->SetBoxExtent(boxExtent);
    boxComp->SetVisibility(false);

    // 4. FRECCIA — "figlia" della root generica
    arrowComp = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComp"));
    arrowComp->SetupAttachment(RootComponent);
    arrowComp->SetArrowColor(FColor::Yellow);
    arrowComp->bIsScreenSizeScaled = true;
}

// ================================================================
// OnConstruction — Aggiorna la vista istantaneamente nell'Editor
// ================================================================
void A_navMeshZone::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // Attiva forma corretta, aggiorna le dimensioni BASE. 
    // La scala dell'Actor (Gizmo) si applica automaticamente in aggiunta a queste!
    if (zoneShape == E_zoneShape::Sphere)
    {
        sphereComp->SetSphereRadius(sphereRadius);
        sphereComp->SetVisibility(true);
        boxComp->SetVisibility(false);
    }
    else
    {
        boxComp->SetBoxExtent(boxExtent);
        boxComp->SetVisibility(true);
        sphereComp->SetVisibility(false);
    }

    ApplyZoneColor();
}

// ================================================================
// BeginPlay — registrazione cache
// ================================================================
void A_navMeshZone::BeginPlay()
{
    Super::BeginPlay();

    // L'aspetto visivo è già stato sistemato da OnConstruction.
    // Qui ci limitiamo ad auto-registrarci nella cache statica.
    if (zoneType == E_navMeshZoneType::Idle)
        allIdleZones.AddUnique(this);
    else
        allMatingZones.AddUnique(this);
}

// ================================================================
// EndPlay — deregistrazione dalla cache
// ================================================================
void A_navMeshZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    allIdleZones.Remove(this);
    allMatingZones.Remove(this);

    Super::EndPlay(EndPlayReason);
}

// ================================================================
// GetNearest — query statica O(n_zone)
// ================================================================
A_navMeshZone* A_navMeshZone::GetNearest(
    UWorld* World,
    FVector Origin,
    E_navMeshZoneType Type)
{
    const TArray<A_navMeshZone*>& ZoneList =
        (Type == E_navMeshZoneType::Idle) ? allIdleZones : allMatingZones;

    A_navMeshZone* Nearest     = nullptr;
    float          NearestDist = FLT_MAX;

    for (A_navMeshZone* Zone : ZoneList)
    {
        if (!IsValid(Zone)) continue;

        const float Dist = FVector::DistSquared(Origin, Zone->GetActorLocation());
        if (Dist < NearestDist)
        {
            NearestDist = Dist;
            Nearest     = Zone;
        }
    }

    return Nearest;
}

// ================================================================
// GetRandomNavPoint — dispatcher per forma attiva
// ================================================================
bool A_navMeshZone::GetRandomNavPoint(FVector& OutLocation) const
{
    return (zoneShape == E_zoneShape::Sphere)
        ? GetRandomNavPoint_Sphere(OutLocation)
        : GetRandomNavPoint_Box(OutLocation);
}

// ================================================================
// GetRandomNavPoint_Sphere
// ================================================================
bool A_navMeshZone::GetRandomNavPoint_Sphere(FVector& OutLocation) const
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return false;

    // MAGIC TRICK: GetScaledSphereRadius calcola (sphereRadius * Scala Actor) automaticamente!
    const float ScaledRadius = sphereComp->GetScaledSphereRadius();

    // Proietta il centro sul NavMesh prima di usarlo come origine —
    // necessario su terreni ondulati dove il pivot dell'actor può essere
    // a una Z diversa dalla superficie NavMesh sottostante.
    FNavLocation ProjectedCenter;
    const FVector QueryExtent(ScaledRadius, ScaledRadius, 500.f);
    if (!NavSys->ProjectPointToNavigation(GetActorLocation(), ProjectedCenter, QueryExtent))
        return false;

    FNavLocation NavLoc;
    const bool bFound = NavSys->GetRandomReachablePointInRadius(
        ProjectedCenter.Location, ScaledRadius, NavLoc);

    if (bFound) OutLocation = NavLoc.Location;
    return bFound;
}

// ================================================================
// GetRandomNavPoint_Box
// ================================================================
bool A_navMeshZone::GetRandomNavPoint_Box(FVector& OutLocation) const
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return false;

    const FVector   Center  = GetActorLocation();
    const FRotator  Rot     = GetActorRotation();
    
    // MAGIC TRICK: Otteniamo gli extent già moltiplicati per la scala dell'Actor
    const FVector ScaledExtent = boxComp->GetScaledBoxExtent();
    const FVector   SnapExt = FVector(50.f, 50.f, ScaledExtent.Z);  // Z = altezza box

    for (int32 Attempt = 0; Attempt < 5; Attempt++)
    {
        // Punto random nel piano XY usando gli extent SCALATI
        const float LocalX = FMath::RandRange(-ScaledExtent.X, ScaledExtent.X);
        const float LocalY = FMath::RandRange(-ScaledExtent.Y, ScaledExtent.Y);

        // Trasforma in world space rispettando rotazione dell'Actor
        const FVector WorldPoint = Center + Rot.RotateVector(FVector(LocalX, LocalY, 0.f));

        FNavLocation NavLoc;
        if (NavSys->ProjectPointToNavigation(WorldPoint, NavLoc, SnapExt))
        {
            OutLocation = NavLoc.Location;
            return true;
        }
    }

    // Fallback: centro della zona con snap esteso
    FNavLocation NavLoc;
    if (NavSys->ProjectPointToNavigation(Center, NavLoc, FVector(100.f, 100.f, ScaledExtent.Z)))
    {
        OutLocation = NavLoc.Location;
        return true;
    }

    return false;
}

// ================================================================
// ApplyZoneColor
// ================================================================
void A_navMeshZone::ApplyZoneColor() const
{
    const FColor ZoneColor = (zoneType == E_navMeshZoneType::Idle)
        ? FColor(100, 180, 255)   // azzurro
        : FColor(255, 80,  80);   // rosso

    sphereComp->ShapeColor = ZoneColor;
    boxComp->ShapeColor    = ZoneColor;
}