#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "NpcMemoryTypes.h"
#include "A_navMeshZone.generated.h"

// ================================================================
// ENUM: Forma della zona
// ================================================================
UENUM(BlueprintType)
enum class E_zoneShape : uint8
{
    Sphere  UMETA(DisplayName = "Sfera"),
    Box     UMETA(DisplayName = "Box")
};

// E_navMeshZoneType è in NpcMemoryTypes.h (incluso sopra)

UCLASS()
class A_navMeshZone : public AActor
{
    GENERATED_BODY()
    
public:    
    // Costruttore
    A_navMeshZone();

    // ================================================================
    // COMPONENTI
    // ================================================================
    
    // La Root rimane visibile per permettere lo spostamento e lo scaling dell'Actor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    // Icona Editor (visibile solo come componente interno, aiuta la selezione 3D)
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    UBillboardComponent* editorIcon;

    // Componenti nascosti dall'interfaccia UI dell'editor per evitare modifiche,
    // ma che rimangono visibili e operativi nel viewport 3D.
    UPROPERTY(BlueprintReadOnly, Category = "Components")
    USphereComponent* sphereComp;

    UPROPERTY(BlueprintReadOnly, Category = "Components")
    UBoxComponent* boxComp;

    UPROPERTY(BlueprintReadOnly, Category = "Components")
    UArrowComponent* arrowComp;

    // ================================================================
    // PROPRIETA' EDITOR
    // ================================================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Settings")
    E_zoneShape zoneShape = E_zoneShape::Sphere;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Settings")
    E_navMeshZoneType zoneType = E_navMeshZoneType::Idle;

    // Mostra il raggio solo se la forma scelta è la Sfera
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Settings", meta = (EditCondition = "zoneShape == E_zoneShape::Sphere", EditConditionHides))
    float sphereRadius = 300.0f;

    // Mostra gli extent solo se la forma scelta è il Box
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Settings", meta = (EditCondition = "zoneShape == E_zoneShape::Box", EditConditionHides))
    FVector boxExtent = FVector(300.0f, 300.0f, 100.0f);


protected:
    // Eventi del ciclo di vita
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ================================================================
    // FUNZIONI PUBBLICHE (Chiamabili da Blueprint)
    // ================================================================
    
    // Restituisce un punto valido sul NavMesh all'interno della zona
    UFUNCTION(BlueprintCallable, Category = "NavZone")
    bool GetRandomNavPoint(FVector& OutLocation) const;

    // Trova la zona più vicina del tipo specificato
    UFUNCTION(BlueprintCallable, Category = "NavZone", meta = (WorldContext = "World"))
    static A_navMeshZone* GetNearest(UWorld* World, FVector Origin, E_navMeshZoneType Type);


private:
    // ================================================================
    // LOGICA INTERNA
    // ================================================================
    bool GetRandomNavPoint_Sphere(FVector& OutLocation) const;
    bool GetRandomNavPoint_Box(FVector& OutLocation) const;
    void ApplyZoneColor() const;

    // ================================================================
    // CACHE STATICA (Condivisa tra tutte le istanze)
    // ================================================================
    static TArray<A_navMeshZone*> allIdleZones;
    static TArray<A_navMeshZone*> allMatingZones;
};